#include "AiAssistant.h"
#include "AiRequest.h"
#include "AiTools.h"
#include "ContentLibrary.h"
#include "ModelInstaller.h"
#include "SearchSpelling.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSettings>
#include <QStandardPaths>

namespace {
bool usable(const QString &runtime, const QString &model) {
    const QFileInfo engine(runtime), weights(model);
    return engine.isFile() && engine.isExecutable() && weights.isFile() && weights.isReadable()
        && weights.size() > 0 && weights.suffix().compare("cact", Qt::CaseInsensitive) == 0;
}

QStringList vocabulary(const ContentLibrary &content, const QString &translation, bool songs) {
    QStringList words;
    if (!songs) {
        for (const auto &book : content.books(translation)) {
            words += SearchSpelling::words(book.toMap()["name"].toString());
        }
        return words;
    }
    for (const auto &value : content.songs()) {
        const auto song = value.toMap();
        words += SearchSpelling::words(song["title"].toString());
        words += SearchSpelling::words(song["tags"].toStringList().join(' '));
        words += SearchSpelling::words(song["tags"].toString());
        for (const auto &section : song["sections"].toList()) {
            words += SearchSpelling::words(section.toMap()["primary"].toString());
            words += SearchSpelling::words(section.toMap()["secondary"].toString());
        }
    }
    words.removeDuplicates();
    return words;
}
}

AiAssistant::AiAssistant(const ContentLibrary *content, QObject *parent, bool preferences)
    : QObject(parent)
    , m_content(content)
    , m_preferences(preferences) {
    if (m_preferences) {
        QSettings settings;
        settings.beginGroup("AiAssistant");
        m_enabled = settings.value("enabled", false).toBool();
        m_runtime = settings.value("runtime").toString();
        m_model = settings.value("model").toString();
        if (m_enabled) {
            m_status = configured() ? "Luma is ready for search requests."
                                    : "Choose a local Cactus runtime and Needle model.";
        }
    }
    connect(&m_backend, &CactusBackend::completed, this,
        [this](const QByteArray &response, const QString &error) {
            if (m_discard) {
                m_discard = false;
            } else if (!error.isEmpty()) {
                m_status = error;
            } else {
                const auto proposal = AiTools::decode(response);
                m_proposals = proposal.actions;
                for (const auto &value : m_proposals) {
                    const auto name = value.toMap()["name"].toString();
                    if ((m_scope == "songs" && name != "search_songs")
                        || (m_scope == "scripture" && name != "search_scripture")) {
                        m_proposals.clear();
                        m_status = "The suggestion did not match the selected search category. Try "
                                   "rephrasing.";
                        emit changed();
                        return;
                    }
                }
                const auto songWords = vocabulary(*m_content, m_translation, true);
                const auto bookWords = vocabulary(*m_content, m_translation, false);
                for (auto &value : m_proposals) {
                    auto action = value.toMap();
                    auto arguments = action["arguments"].toMap();
                    const auto original = arguments["query"].toString();
                    const bool songSearch = action["name"] == "search_songs";
                    // Exact saved-library matches always win over a spelling suggestion.
                    const bool exact = songSearch
                        ? !m_content->searchSongs(original).isEmpty()
                        : !m_content->searchVerses(m_translation, original).isEmpty();
                    const auto corrected = exact
                        ? original
                        : SearchSpelling::correct(original, songSearch ? songWords : bookWords,
                              songSearch ? SearchSpelling::Vocabulary::Songs
                                         : SearchSpelling::Vocabulary::BibleBooks);
                    arguments["query"] = corrected;
                    if (corrected != original) {
                        action["originalQuery"] = original;
                    }
                    if (action["name"] == "search_scripture") {
                        arguments["translation_id"] = m_translation;
                    }
                    action["arguments"] = arguments;
                    value = action;
                }
                m_status = !proposal.error.isEmpty() ? proposal.error
                    : proposal.actions.isEmpty()
                    ? "No supported search found. Try a song or Bible reference."
                    : "Review the suggested searches before running them.";
            }
            emit changed();
        });
}

bool AiAssistant::configured() const {
    return usable(m_runtime, m_model);
}

void AiAssistant::persist() {
    if (!m_preferences) {
        return;
    }
    QSettings settings;
    settings.beginGroup("AiAssistant");
    settings.setValue("enabled", m_enabled);
    settings.setValue("runtime", m_runtime);
    settings.setValue("model", m_model);
}

bool AiAssistant::configure(const QUrl &runtime, const QUrl &model) {
    if (busy() || !runtime.isLocalFile() || !model.isLocalFile()
        || !usable(runtime.toLocalFile(), model.toLocalFile())) {
        m_status = "Choose an executable Cactus runtime and a readable .cact model file.";
        emit changed();
        return false;
    }
    m_runtime = QFileInfo(runtime.toLocalFile()).canonicalFilePath();
    m_model = QFileInfo(model.toLocalFile()).canonicalFilePath();
    m_proposals.clear();
    m_results.clear();
    persist();
    m_status = m_enabled ? "Luma is ready for search requests." : "Configured. Luma is off.";
    emit changed();
    return true;
}

void AiAssistant::setEnabled(bool enabled) {
    if (enabled && !configured()) {
        m_status = "Choose a local Cactus runtime and Needle model first.";
        emit changed();
        return;
    }
    if (!enabled) {
        m_discard = busy();
        m_backend.cancel();
        m_proposals.clear();
        m_results.clear();
    }
    m_enabled = enabled;
    persist();
    m_status = enabled ? "Luma is ready for search requests." : "Luma is off.";
    emit changed();
}

bool AiAssistant::useManagedModel() {
    if (busy()) {
        return false;
    }
    if (!ModelInstaller::managedReady()) {
        m_status = "Download or repair Luma in Setup first. Managed files must pass verification.";
        emit changed();
        return false;
    }
    const QDir folder(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ai/cactus-4.0.0");
#ifdef Q_OS_WIN
    const auto executable = folder.filePath("cactus.exe");
#else
    const auto executable = folder.filePath("cactus");
#endif
    if (!configure(QUrl::fromLocalFile(executable),
            QUrl::fromLocalFile(folder.filePath("needle3.cact")))) {
        return false;
    }
    setEnabled(true);
    return enabled();
}

void AiAssistant::propose(
    const QString &prompt, const QString &translationId, const QString &scope) {
    if (busy()) {
        return;
    }
    m_proposals.clear();
    m_results.clear();
    if (!m_enabled || !configured() || !m_content) {
        m_status = "Enable and configure Luma before sending a request.";
    } else if (scope != "all" && scope != "songs" && scope != "scripture") {
        m_status = "Choose Songs or Bible as the search category.";
    } else if (prompt.trimmed().isEmpty() || prompt.size() > 2000 || prompt.contains(QChar::Null)) {
        m_status = "Enter a request of 1 to 2000 characters.";
    } else {
        m_scope = scope;
        m_translation = translationId;
        if (m_translation.isEmpty() && !m_content->translations().isEmpty()) {
            m_translation = m_content->translations().first().toMap()["id"].toString();
        }
        m_discard = false;
        m_status = "Preparing search suggestions…";
        const auto context
            = QByteArray("Extract search terms or a Bible reference from the request. "
                         "Tolerate spelling mistakes and correct obvious typos in search terms. "
                         "Only propose the supplied search tools. Never generate scripture or "
                         "change presentation output.");
        // Help the small router recognize misspelled book names before inference.
        // Chapters, verse numbers and punctuation are preserved verbatim.
        const auto correctedPrompt = scope == "scripture"
            ? SearchSpelling::correct(prompt, vocabulary(*m_content, m_translation, false),
                  SearchSpelling::Vocabulary::BibleBooks)
            : prompt;
        const auto resolved = scope == "scripture"
            ? m_content->resolveReference(m_translation, prompt)
            : QVariantMap();
        const AiRequest::Parsed reference { resolved["handled"].toBool(),
            resolved["query"].toString(), resolved["error"].toString() };
        const auto songTerms = scope == "songs" ? AiRequest::teluguSongTerms(prompt) : QString();
        if (reference.handled || !songTerms.isEmpty()) {
            const auto query = reference.handled ? reference.query : songTerms;
            if (query.size() > 256) {
                m_status = "Use at most 256 characters for the search terms.";
            } else if (!reference.error.isEmpty()) {
                m_status = reference.error;
            } else if (reference.handled
                && m_content->searchVerses(m_translation, query).isEmpty()) {
                m_status = "That reference is unavailable in the selected Bible. Check the "
                           "chapter, verses or translation.";
            } else {
                QVariantMap arguments { { "query", query } };
                if (reference.handled) {
                    arguments["translation_id"] = m_translation;
                }
                m_proposals = { QVariantMap {
                    { "name", reference.handled ? "search_scripture" : "search_songs" },
                    { "arguments", arguments }, { "origin", "request" }, { "confidence", 1.0 } } };
                m_status
                    = "Search extracted from your request. Review it before searching the library.";
            }
        } else {
            m_backend.request(m_runtime, m_model, correctedPrompt, context, scope);
        }
    }
    emit changed();
}

bool AiAssistant::runSearches() {
    if (!m_enabled || busy() || m_proposals.isEmpty() || !m_content) {
        return false;
    }
    const auto translations = m_content->translations();
    // Validate the whole proposal before performing even its first read.
    for (const auto &value : m_proposals) {
        const auto action = value.toMap();
        if (action["name"] != "search_scripture") {
            continue;
        }
        const auto id = action["arguments"].toMap()["translation_id"].toString();
        bool installed = false;
        for (const auto &translation : translations) {
            installed |= translation.toMap()["id"].toString() == id;
        }
        if (!installed) {
            m_status
                = "A suggested Bible is not installed. Rephrase with an installed translation.";
            m_proposals.clear();
            emit changed();
            return false;
        }
    }
    QVariantList groups;
    int shown = 0;
    for (const auto &value : m_proposals) {
        const auto action = value.toMap();
        const auto arguments = action["arguments"].toMap();
        const auto query = arguments["query"].toString();
        const auto rows = action["name"] == "search_songs"
            ? m_content->searchSongs(query)
            : m_content->searchVerses(arguments["translation_id"].toString(), query);
        shown += rows.size() > 50 ? 50 : int(rows.size());
        groups.append(QVariantMap { { "tool", action["name"] }, { "query", query },
            { "items", rows.mid(0, 50) }, { "total", rows.size() } });
    }
    m_results = groups;
    m_proposals.clear();
    m_status = shown ? QString("Showing %1 results from your saved library.").arg(shown)
                     : "No matches found in your saved library.";
    emit changed();
    return true;
}

void AiAssistant::cancel() {
    m_discard = busy();
    m_backend.cancel();
    m_proposals.clear();
    m_results.clear();
    m_status = "Assistant request cancelled.";
    emit changed();
}
