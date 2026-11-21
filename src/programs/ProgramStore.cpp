#include "ProgramStore.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramSlides.h"
#include "SlideFactory.h"
#include <QDate>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {
QString uid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

}

void ProgramStore::setSelected(int index) {
    if (index < 0 || index >= items().size() || index == m_selected) {
        return;
    }
    m_selected = index;
    emit changed();
}

QVariantMap ProgramStore::selectedItem() const {
    auto list = items();
    return m_selected >= 0 && m_selected < list.size() ? list[m_selected].toMap() : QVariantMap();
}

QVariantList ProgramStore::selectedSlides() const {
    return renderItem(selectedItem());
}

bool ProgramStore::live() const {
    return m_outputs && !id().isEmpty() && m_outputs->liveSource() == "program:" + id();
}

QString ProgramStore::liveItemId() const {
    return live() ? m_outputs->liveProgramItemId() : QString();
}

void ProgramStore::checkpoint() {
    m_undo << m_program;
    if (m_undo.size() > 40) {
        m_undo.removeFirst();
    }
    m_redo.clear();
}

void ProgramStore::commit() {
    m_dirty = true;
    m_status = "Saving program…";
    m_autosave.start();
    emit changed();
    emit liveChanged();
}

void ProgramStore::updateDetails(const QString &name, const QString &date) {
    if (id().isEmpty() || (title() == name && this->date() == date)) {
        return;
    }
    if (name.trimmed().isEmpty() || name.size() > 200
        || (!date.isEmpty() && !QDate::fromString(date, Qt::ISODate).isValid())) {
        statusMessage("Enter a title and a valid date (YYYY-MM-DD)");
        return;
    }
    checkpoint();
    m_program["title"] = name.trimmed();
    m_program["date"] = date;
    commit();
}

bool ProgramStore::append(
    QVariantMap item, const QVariantList &slides, const QString &error, const QString &programId) {
    const bool elsewhere = !programId.isEmpty() && programId != id();
    if (!elsewhere && id().isEmpty()) {
        statusMessage("Create or open a program first");
        return false;
    }
    if (!error.isEmpty()) {
        statusMessage(error);
        return false;
    }
    auto valid = EditorDocument::validatedSlides(slides);
    if (valid.isEmpty()) {
        statusMessage("No valid slides to add");
        return false;
    }
    // Target another saved program without changing the editor, selection, history or last-opened
    // preference.
    auto candidate = m_program;
    bool transactionOpen = false;
    auto rollback = qScopeGuard([&] {
        if (transactionOpen) {
            m_db.rollback();
        }
    });
    if (elsewhere) {
        if (!m_db.transaction()) {
            statusMessage("Could not open the destination program for editing");
            return false;
        }
        transactionOpen = true;
        QSqlQuery query(m_db);
        query.prepare("SELECT data FROM programs WHERE id=?");
        query.addBindValue(programId);
        if (!query.exec() || !query.next()) {
            statusMessage("Destination program not found");
            return false;
        }
        candidate = QJsonDocument::fromJson(query.value(0).toByteArray()).object().toVariantMap();
        if (candidate["id"] != programId || !validate(candidate, false)) {
            statusMessage("Destination program is invalid; nothing added");
            return false;
        }
    }
    auto list = candidate["items"].toList();
    int slideCount = 0;
    for (const auto &value : list) {
        slideCount += value.toMap()["slides"].toList().size();
    }
    if (list.size() >= 200 || slideCount + valid.size() > 2000) {
        statusMessage("A program can contain 200 items and 2,000 slides");
        return false;
    }
    item["id"] = uid();
    for (int i = 0; i < valid.size(); i++) {
        auto page = valid[i].toMap();
        if (!page.contains("ambientOpacity")) {
            page["ambientOpacity"] = i == 0 ? 0.85 : 0.5;
        }
        valid[i] = page;
    }
    item["slides"] = ProgramSlides::cloneWithFreshIds(valid);
    item["notes"] = "";
    item["template"] = "original";
    list << item;
    candidate["items"] = list;
    if (elsewhere) {
        QSqlQuery query(m_db);
        query.prepare("UPDATE programs SET data=?,updated=unixepoch() WHERE id=?");
        query.addBindValue(QString::fromUtf8(
            QJsonDocument::fromVariant(candidate).toJson(QJsonDocument::Compact)));
        query.addBindValue(programId);
        if (!query.exec() || query.numRowsAffected() != 1 || !m_db.commit()) {
            statusMessage("Could not save the destination program; nothing added");
            return false;
        }
        transactionOpen = false;
        statusMessage("Added to " + candidate["title"].toString());
    } else {
        checkpoint();
        m_program = candidate;
        m_selected = list.size() - 1;
        commit();
    }
    return true;
}

bool ProgramStore::addSong(const QVariantMap &song, int maxLines, const QVariantMap &definition,
    const QString &programId) {
    QString error;
    const auto slides = SlideFactory::songSlides(song, maxLines, &error, definition);
    return append({ { "kind", "song" }, { "title", song["title"] }, { "song", song },
                      { "maxLines", maxLines }, { "layoutTemplate", definition } },
        slides, error, programId);
}

bool ProgramStore::addScripture(const QVariantList &verses, int maxLines,
    const QVariantMap &definition, const QString &programId) {
    QString error;
    const auto slides = SlideFactory::scriptureSlides(verses, maxLines, &error, definition);
    QString title = "Bible passage";
    if (!verses.isEmpty()) {
        title = verses.first().toMap()["reference"].toString();
        if (verses.size() > 1) {
            title += " – " + verses.last().toMap()["reference"].toString();
        }
        title += " · " + verses.first().toMap()["abbreviation"].toString();
    }
    return append({ { "kind", "scripture" }, { "title", title }, { "maxLines", maxLines },
                      { "verses", verses }, { "layoutTemplate", definition } },
        slides, error, programId);
}

bool ProgramStore::addPresentation(
    const QVariantList &slides, const QString &title, const QString &programId) {
    return append(
        { { "kind", "presentation" },
            { "title", title.trimmed().isEmpty() ? "Custom presentation" : title.left(200) } },
        slides, {}, programId);
}

bool ProgramStore::importPresentation(const QUrl &url) {
    QFile f(url.toLocalFile());
    if (!url.isLocalFile() || !f.open(QIODevice::ReadOnly) || f.size() > 50 * 1024 * 1024) {
        statusMessage("Choose a local Selah project under 50 MB");
        return false;
    }
    const auto root = QJsonDocument::fromJson(f.readAll()).object();
    if (root["format"].toInt() != 1) {
        statusMessage("Invalid Selah project");
        return false;
    }
    return addPresentation(
        root["slides"].toArray().toVariantList(), QFileInfo(f).completeBaseName());
}

bool ProgramStore::addMedia(const QVariantMap &asset, const QString &programId) {
    const QUrl source(asset["source"].toString());
    const auto kind = asset["kind"].toString();
    if (!source.isLocalFile() || !QFileInfo::exists(source.toLocalFile())
        || (kind != "image" && kind != "video")) {
        statusMessage("Choose an available image or video from the media library");
        return false;
    }
    QVariantMap element { { "uid", uid() }, { "kind", kind }, { "name", asset["name"] },
        { "source", source.toString() }, { "fit", "contain" }, { "loop", true }, { "muted", true },
        { "x", 0 }, { "y", 0 }, { "width", 1920 }, { "height", 1080 }, { "fontSize", 36 },
        { "color", "#FFFFFF" }, { "opacity", 1.0 }, { "rotation", 0 }, { "visible", true },
        { "locked", false } };
    return append({ { "kind", "media" }, { "title", asset["name"] } },
        QVariantList { QVariantMap { { "name", asset["name"] }, { "background", "#000000" },
            { "ambientOpacity", 0.0 }, { "notes", "" },
            { "elements", QVariantList { element } } } },
        {}, programId);
}

void ProgramStore::replaceItem(const QVariantMap &item) {
    auto list = items();
    list[m_selected] = item;
    m_program["items"] = list;
    commit();
}

void ProgramStore::moveItem(int delta) {
    moveItemTo(m_selected, m_selected + delta);
}

bool ProgramStore::moveItemTo(int from, int to) {
    auto list = items();
    if (from < 0 || to < 0 || from >= list.size() || to >= list.size() || from == to) {
        return false;
    }
    const auto selection = selectedItem().value("id").toString();
    checkpoint();
    list.move(from, to);
    m_program["items"] = list;
    restoreSelection(selection);
    commit();
    return true;
}

void ProgramStore::removeSelected() {
    auto list = items();
    if (m_selected < 0 || m_selected >= list.size()) {
        return;
    }
    checkpoint();
    list.removeAt(m_selected);
    m_selected = qMin(m_selected, int(list.size()) - 1);
    m_program["items"] = list;
    commit();
}

void ProgramStore::setItemDetails(const QString &title, const QString &notes) {
    auto item = selectedItem();
    if (item.isEmpty() || (item["title"] == title && item["notes"] == notes)) {
        return;
    }
    if (title.trimmed().isEmpty() || title.size() > 200 || notes.size() > 10000) {
        statusMessage("Enter an item title; notes may contain up to 10,000 characters");
        return;
    }
    checkpoint();
    item["title"] = title.trimmed();
    item["notes"] = notes;
    replaceItem(item);
}

bool ProgramStore::setArrangement(int index) {
    auto item = selectedItem();
    if (item["kind"] != "song") {
        return false;
    }
    auto song = item["song"].toMap();
    const auto choices = song["arrangements"].toList();
    if (index < 0 || index >= choices.size()) {
        statusMessage("Choose a valid arrangement");
        return false;
    }
    song["activeArrangement"] = index;
    song["arrangement"] = choices[index].toMap()["order"];
    QString error;
    const auto slides = SlideFactory::songSlides(
        song, item["maxLines"].toInt(), &error, item["layoutTemplate"].toMap());
    if (!error.isEmpty() || slides.isEmpty()) {
        statusMessage(error);
        return false;
    }
    if (flatten().size() - item["slides"].toList().size() + slides.size() > 2000) {
        statusMessage("This arrangement would exceed 2,000 slides");
        return false;
    }
    checkpoint();
    item["song"] = song;
    item["slides"] = ProgramSlides::cloneWithFreshIds(slides);
    replaceItem(item);
    return true;
}

void ProgramStore::setTemplate(const QString &name) {
    auto item = selectedItem();
    if (item.isEmpty() || item["template"] == name
        || !QStringList { "original", "midnight", "paper" }.contains(name)) {
        return;
    }
    if (item["kind"] != "song" && item["kind"] != "scripture") {
        return;
    }
    checkpoint();
    item["template"] = name;
    replaceItem(item);
}

bool ProgramStore::updateSelectedSlides(const QVariantList &slides) {
    auto item = selectedItem();
    auto valid = EditorDocument::validatedSlides(slides);
    if (item.isEmpty() || valid.isEmpty()) {
        statusMessage("Select an item with valid editor slides");
        return false;
    }
    if (flatten().size() - item["slides"].toList().size() + valid.size() > 2000) {
        statusMessage("A program can contain at most 2,000 slides");
        return false;
    }
    checkpoint();
    item["slides"] = ProgramSlides::cloneWithFreshIds(valid);
    item["template"] = "original";
    replaceItem(item);
    return true;
}

QVariantList ProgramStore::renderItem(const QVariantMap &item) {
    auto slides = item["slides"].toList();
    const auto skin = item.value("template", "original").toString();
    for (auto &v : slides) {
        auto page = v.toMap();
        if (skin != "original") {
            page["background"] = skin == "paper" ? "#EEEAE0" : "#101820";
            page["ambientOpacity"] = 0.0;
            auto elements = page["elements"].toList();
            for (auto &e : elements) {
                auto el = e.toMap();
                if (el["kind"] == "text") {
                    el["color"] = skin == "paper" ? "#18212B" : "#F4F3ED";
                }
                e = el;
            }
            page["elements"] = elements;
        }
        const auto notes = item["notes"].toString();
        if (!notes.isEmpty()) {
            page["notes"] = notes
                + (page["notes"].toString().isEmpty() ? "" : "\n\n" + page["notes"].toString());
        }
        v = page;
    }
    return slides;
}

QVariantList ProgramStore::flatten(int *selectedStart) const {
    // Live item tracking uses identity, so later draft reordering cannot move the live marker.
    QVariantList result;
    const auto list = items();
    if (selectedStart) {
        *selectedStart = -1;
    }
    for (int i = 0; i < list.size(); i++) {
        const auto item = list[i].toMap();
        if (selectedStart && i == m_selected) {
            *selectedStart = result.size();
        }
        for (auto slide : renderItem(item)) {
            auto page = slide.toMap();
            page["programItemId"] = item["id"];
            page["programItemTitle"] = item["title"];
            page["programTitle"] = title();
            result << page;
        }
    }
    return result;
}

bool ProgramStore::present(int localSlide) {
    if (!m_outputs) {
        return false;
    }
    int start = -1;
    const auto slides = flatten(&start);
    if (start < 0 || localSlide < 0 || localSlide >= selectedSlides().size()) {
        statusMessage("Select an item and a slide to present");
        return false;
    }
    if (m_dirty && !save()) {
        return false;
    }
    m_outputs->takeLive(slides, start + localSlide, "program:" + id());
    statusMessage("Program snapshot taken live");
    return true;
}

void ProgramStore::restoreSelection(const QString &itemId) {
    const auto list = items();
    for (int index = 0; index < list.size(); ++index) {
        if (list[index].toMap().value("id").toString() == itemId) {
            m_selected = index;
            return;
        }
    }
    m_selected = qBound(-1, m_selected, int(list.size()) - 1);
}

void ProgramStore::undo() {
    if (m_undo.isEmpty()) {
        return;
    }
    const auto selection = selectedItem().value("id").toString();
    m_redo << m_program;
    m_program = m_undo.takeLast();
    restoreSelection(selection);
    commit();
}

void ProgramStore::redo() {
    if (m_redo.isEmpty()) {
        return;
    }
    const auto selection = selectedItem().value("id").toString();
    m_undo << m_program;
    m_program = m_redo.takeLast();
    restoreSelection(selection);
    commit();
}

QStringList ProgramStore::warnings() const {
    QSet<QString> missing;
    for (const auto &item : items()) {
        for (const auto &slide : item.toMap()["slides"].toList()) {
            for (const auto &element : slide.toMap()["elements"].toList()) {
                const auto e = element.toMap();
                if (e["kind"] == "image" || e["kind"] == "video") {
                    const QUrl source(e["source"].toString());
                    if (!QFileInfo::exists(source.toLocalFile())) {
                        missing.insert(QFileInfo(source.toLocalFile()).fileName());
                    }
                }
            }
        }
    }
    auto result = missing.values();
    result.sort();
    return result;
}
