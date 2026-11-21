#include "EditorDocument.h"
#include "DocumentElements.h"
#include "SlideFactory.h"
#include "TextFormatting.h"
#include <algorithm>
#include <cmath>
#include <QColor>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

namespace {

QVariantMap slide(const QString &name, const QString &headline, const QString &subtitle,
    const QString &background) {
    return { { "name", name }, { "background", background }, { "notes", "" },
        { "elements",
            QVariantList { DocumentElements::text("Gathering label",
                               "S U N D A Y   G A T H E R I N G", 160, 128, 1500, 70, 26, "Arial"),
                DocumentElements::text(
                    "Welcome headline", headline, 150, 282, 1550, 440, 174, "Georgia"),
                DocumentElements::text(
                    "Service details", subtitle, 160, 782, 1500, 80, 36, "Arial", "#DDD9E7"),
                DocumentElements::text("Church signature",
                    "CHURCH COMMUNITY     /     SUNDAYS AT 09:30 AM", 160, 963, 1600, 55, 22,
                    "Arial", "#CEC6DE") } } };
}
}

EditorDocument::EditorDocument(QObject *parent, bool recover)
    : QObject(parent) {
    m_slides = { slide("Welcome", "You belong\nhere.", "A new morning. A shared hope.", "#54435F"),
        slide("This week", "Life\ntogether.", "Prayer night · Wednesday, 7 PM", "#355147"),
        slide("Connect", "Let’s\nconnect.", "Find your community this Sunday.", "#705443") };
    m_autosave.setSingleShot(true);
    m_autosave.setInterval(650);
    connect(&m_autosave, &QTimer::timeout, this, [this] { saveProject(); });
    if (recover) {
        QFile file(recoveryPath());
        if (file.open(QIODevice::ReadOnly)) {
            restore(QJsonDocument::fromJson(file.readAll()).object());
        }
    }
}

QVariantList EditorDocument::elements() const {
    return m_slides[m_current].toMap()["elements"].toList();
}

QVariantList EditorDocument::slides() const {
    return m_slides;
}

QString EditorDocument::notes() const {
    return m_slides[m_current].toMap()["notes"].toString();
}

QString EditorDocument::background() const {
    return m_slides[m_current].toMap()["background"].toString();
}

void EditorDocument::setCurrentSlide(int i) {
    if (i < 0 || i >= m_slides.size() || i == m_current) {
        return;
    }
    m_current = i;
    m_selected = -1;
    emit changed();
    emit selectionChanged();
}

void EditorDocument::setSelected(int i) {
    if (i < -1 || i >= elements().size() || i == m_selected) {
        return;
    }
    m_selected = i;
    emit selectionChanged();
}

QVariantMap EditorDocument::selectedElement() const {
    auto list = elements();
    return m_selected >= 0 && m_selected < list.size() ? list[m_selected].toMap() : QVariantMap();
}

void EditorDocument::checkpoint() {
    m_undo.append(snapshot());
    if (m_undo.size() > 100) {
        m_undo.removeFirst();
    }
    m_redo.clear();
}

void EditorDocument::commit() {
    emit changed();
    emit selectionChanged();
    setStatus("Saving changes…");
    m_autosave.start();
}

void EditorDocument::setStatus(const QString &s) {
    m_status = s;
    emit statusChanged();
}

void EditorDocument::formatText(const QVariantMap &patch, int start, int end) {
    const auto element = selectedElement();
    if (element.isEmpty() || element.value("kind", "text") != "text") {
        return;
    }
    TextFormatting formatter;
    updateElement(m_selected, formatter.apply(element, patch, start, end));
}

void EditorDocument::updateElement(int i, const QVariantMap &patch) {
    auto list = elements();
    if (i < 0 || i >= list.size()) {
        return;
    }
    auto el = list[i].toMap();
    if (el["locked"].toBool() && !patch.contains("locked")) {
        return;
    }
    auto before = el;
    if (el.value("kind", "text") == "text") {
        el = TextFormatting::normalize(el);
    }
    for (auto it = patch.begin(); it != patch.end(); ++it) {
        if (!el.contains(it.key()) || QStringList { "uid", "kind", "source" }.contains(it.key())) {
            continue;
        }
        if (it.key() == "fit"
            && !QStringList { "contain", "cover", "stretch" }.contains(it.value().toString())) {
            continue;
        }
        if (it.key() == "alignment"
            && !QStringList { "left", "center", "right", "justify" }.contains(
                it.value().toString())) {
            continue;
        }
        if (QStringList { "x", "y", "width", "height", "fontSize", "rotation", "opacity" }.contains(
                it.key())) {
            bool ok = false;
            double n = it.value().toDouble(&ok);
            if (!ok || !std::isfinite(n)) {
                continue;
            }
            if (it.key() == "x") {
                n = std::clamp(n, 0.0, 1880.0);
            }
            if (it.key() == "y") {
                n = std::clamp(n, 0.0, 1050.0);
            }
            if (it.key() == "width") {
                n = std::clamp(n, 40.0, 1920.0);
            }
            if (it.key() == "height") {
                n = std::clamp(n, 30.0, 1080.0);
            }
            if (it.key() == "fontSize") {
                n = std::clamp(n, 8.0, 400.0);
            }
            if (it.key() == "opacity") {
                n = std::clamp(n, 0.0, 1.0);
            }
            el[it.key()] = n;
        } else if (it.key() == "color") {
            if (QColor(it.value().toString()).isValid()) {
                el[it.key()] = it.value();
            }
        } else {
            el[it.key()] = it.value();
        }
    }
    if (el.value("kind", "text") == "text") {
        el = TextFormatting::normalize(el);
    }
    if (el == before) {
        return;
    }
    checkpoint();
    list[i] = el;
    auto s = m_slides[m_current].toMap();
    s["elements"] = list;
    m_slides[m_current] = s;
    commit();
}

void EditorDocument::moveSelected(int delta) {
    if (m_selected < 0 || elements().isEmpty()) {
        return;
    }
    moveElementTo(m_selected, std::clamp(m_selected + delta, 0, int(elements().size()) - 1));
}

bool EditorDocument::moveElementTo(int from, int to) {
    auto list = elements();
    if (from < 0 || to < 0 || from >= list.size() || to >= list.size() || from == to) {
        return false;
    }
    const auto selectedId = selectedElement().value("uid").toString();
    checkpoint();
    list.move(from, to);
    auto slide = m_slides[m_current].toMap();
    slide["elements"] = list;
    m_slides[m_current] = slide;
    m_selected = -1;
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].toMap().value("uid").toString() == selectedId) {
            m_selected = i;
            break;
        }
    }
    commit();
    return true;
}

void EditorDocument::setNotes(const QString &value) {
    auto s = m_slides[m_current].toMap();
    if (s["notes"].toString() == value) {
        return;
    }
    checkpoint();
    s["notes"] = value;
    m_slides[m_current] = s;
    commit();
}

void EditorDocument::setBackground(const QString &value) {
    if (!QColor(value).isValid() || value == background()) {
        return;
    }
    checkpoint();
    auto s = m_slides[m_current].toMap();
    s["background"] = value;
    m_slides[m_current] = s;
    commit();
}

void EditorDocument::addText() {
    checkpoint();
    auto list = elements();
    list.append(
        DocumentElements::text("New text", "Your text here", 240, 440, 1300, 160, 96, "Georgia"));
    auto s = m_slides[m_current].toMap();
    s["elements"] = list;
    m_slides[m_current] = s;
    m_selected = int(list.size()) - 1;
    commit();
}

void EditorDocument::duplicateSelected() {
    if (m_selected < 0) {
        return;
    }
    auto el = selectedElement();
    checkpoint();
    el["uid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    el["x"] = std::min(el["x"].toDouble() + 30, 1880.0);
    el["y"] = std::min(el["y"].toDouble() + 30, 1050.0);
    el["name"] = el["name"].toString() + " copy";
    auto list = elements();
    list.append(el);
    auto s = m_slides[m_current].toMap();
    s["elements"] = list;
    m_slides[m_current] = s;
    m_selected = int(list.size()) - 1;
    commit();
}

void EditorDocument::deleteSelected() {
    if (m_selected < 0 || selectedElement()["locked"].toBool()) {
        return;
    }
    checkpoint();
    auto list = elements();
    list.removeAt(m_selected);
    auto s = m_slides[m_current].toMap();
    s["elements"] = list;
    m_slides[m_current] = s;
    m_selected = -1;
    commit();
}

void EditorDocument::addSlide() {
    checkpoint();
    m_slides.append(
        slide("Untitled", "A fresh\nbeginning.", "Add your service details.", "#54435F"));
    m_current = int(m_slides.size()) - 1;
    m_selected = 1;
    commit();
}

bool EditorDocument::appendGenerated(const QVariantList &slides, const QString &error) {
    if (!error.isEmpty() || slides.isEmpty()) {
        setStatus(error.isEmpty() ? "No slides to add" : error);
        return false;
    }
    if (m_slides.size() + slides.size() > 500) {
        setStatus("This presentation would exceed 500 slides");
        return false;
    }
    checkpoint();
    const int first = m_slides.size();
    m_slides += slides;
    m_current = first;
    m_selected = -1;
    commit();
    return true;
}

bool EditorDocument::addSongSlides(
    const QVariantMap &song, int maxLines, const QVariantMap &definition) {
    QString error;
    const auto pages = SlideFactory::songSlides(song, maxLines, &error, definition);
    return appendGenerated(pages, error);
}

bool EditorDocument::addScriptureSlides(
    const QVariantList &verses, int maxLines, const QVariantMap &definition) {
    QString error;
    const auto pages = SlideFactory::scriptureSlides(verses, maxLines, &error, definition);
    return appendGenerated(pages, error);
}

void EditorDocument::undo() {
    if (!canUndo()) {
        return;
    }
    m_redo.append(snapshot());
    restore(m_undo.takeLast());
    commit();
}

void EditorDocument::redo() {
    if (!canRedo()) {
        return;
    }
    m_undo.append(snapshot());
    restore(m_redo.takeLast());
    commit();
}

bool EditorDocument::replaceSlides(const QVariantList &slides) {
    const auto validated = validatedSlides(slides);
    if (validated.isEmpty()) {
        setStatus("This item has no valid slides");
        return false;
    }
    checkpoint();
    restore({ { "format", 1 }, { "slides", QJsonArray::fromVariantList(validated) } });
    commit();
    return true;
}
