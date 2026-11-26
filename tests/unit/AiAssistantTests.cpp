#include "AiAssistant.h"
#include "AiTools.h"
#include "ContentLibrary.h"
#include "SearchSpelling.h"
#include <functional>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QThread>

namespace {
bool wait(const std::function<bool()> &done) {
    QElapsedTimer deadline;
    deadline.start();
    while (!done() && deadline.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(2);
    }
    return done();
}

QByteArray response(const QString &name, const QJsonObject &arguments, double confidence = 0.95) {
    return QJsonDocument(
        QJsonObject { { "success", true }, { "confidence", confidence },
            { "function_calls",
                QJsonArray { QJsonObject { { "name", name }, { "arguments", arguments } } } } })
        .toJson(QJsonDocument::Compact);
}
}

int runAiAssistantChecks() {
    const QStringList books { "genesis", "psalms", "matthew", "revelation", "john" };
    for (const auto &book : books) {
        QStringList variants;
        for (int i = 0; i < book.size(); ++i) {
            auto missing = book;
            missing.remove(i, 1);
            variants.append(missing);
            auto extra = book;
            extra.insert(i, 'x');
            variants.append(extra);
            if (i + 1 < book.size()) {
                auto swapped = book;
                const QChar letter = swapped[i];
                swapped[i] = swapped[i + 1];
                swapped[i + 1] = letter;
                variants.append(swapped);
            }
        }
        for (const auto &variant : variants) {
            if (SearchSpelling::correct(
                    variant + " 1:1-3", books, SearchSpelling::Vocabulary::BibleBooks)
                != book + " 1:1-3") {
                qWarning() << "Book spelling correction failed" << variant << book;
                return 22;
            }
        }
    }
    if (SearchSpelling::correct("genses 1:1", { "Genesis" }) != "genesis 1:1") {
        return 21;
    }
    const QStringList dictionary { "Amazing", "Grace", "Genesis", "John", "light", "night" };
    for (const auto &pair : QList<QPair<QString, QString>> { { "amzing grcae", "amazing grace" },
             { "Genisis 1:1-3", "genesis 1:1-3" }, { "Jhon 3:16", "john 3:16" },
             { "right", "right" }, { "Grace", "Grace" }, { "xyzabc", "xyzabc" },
             { "నీ కృప", "నీ కృప" } }) {
        if (SearchSpelling::correct(pair.first, dictionary) != pair.second) {
            return 18;
        }
    }
    if (AiTools::schemas("songs").size() != 1 || AiTools::schemas("scripture").size() != 1
        || !AiTools::schemas("unsupported").isEmpty()) {
        return 15;
    }
    const QJsonObject query { { "query", "నీ కృప" } };
    if (!AiTools::decode(response("search_songs", query)).error.isEmpty()
        || AiTools::decode(response("search_songs", query)).actions.size() != 1) {
        return 1;
    }
    for (const auto &invalid : { QByteArray("not JSON"), response("take_live", query),
             response("search_songs", query, 0.03), response("search_songs", {}),
             response("search_songs", { { "query", 12 } }),
             response("search_songs", { { "query", "faith" }, { "shell", "anything" } }) }) {
        const auto proposal = AiTools::decode(invalid);
        if (proposal.error.isEmpty() || !proposal.actions.isEmpty()) {
            return 2;
        }
    }
    const auto mixed = QByteArray("{\"success\":true,\"confidence\":0.9,\"function_calls\":[")
        + "{\"name\":\"search_songs\",\"arguments\":{\"query\":\"faith\"}},"
          "{\"name\":\"take_live\",\"arguments\":{}}]}";
    if (!AiTools::decode(mixed).actions.isEmpty()) {
        return 3;
    }

    QTemporaryDir directory;
    const QString runtime = QString::fromUtf8(SELAH_AI_TEST_RUNTIME_PATH);
    const auto model = directory.filePath("fixture.cact");
    QFile weights(model);
    if (!weights.open(QIODevice::WriteOnly) || weights.write("fixture") != 7) {
        return 4;
    }
    weights.close();
    ContentLibrary content(nullptr, directory.filePath("content"));
    if (content
            .saveSong({ { "title", "Gather in Your Light" }, { "primaryLanguage", "English" },
                { "sections",
                    QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                        { "primary", "We gather in Your light" } } } },
                { "arrangement", QVariantList { "v1" } } })
            .isEmpty()) {
        return 20;
    }
    const auto original = content.songs();
    AiAssistant assistant(&content, nullptr, false);
    assistant.propose("faith");
    if (assistant.enabled() || assistant.busy() || !assistant.proposals().isEmpty()
        || !assistant.configure(QUrl::fromLocalFile(runtime), QUrl::fromLocalFile(model))) {
        return 5;
    }
    assistant.setEnabled(true);
    assistant.propose("gatther", "", "songs");
    if (!wait([&] { return !assistant.busy(); }) || assistant.proposals().isEmpty()
        || assistant.proposals().first().toMap()["arguments"].toMap()["query"] != "gather"
        || assistant.proposals().first().toMap()["originalQuery"] != "gatther"
        || !assistant.results().isEmpty() || !assistant.runSearches()
        || assistant.results().first().toMap()["items"].toList().isEmpty()) {
        return 19;
    }
    const QString prompt = "నీ కృప ; $(literal text)";
    assistant.propose(prompt);
    if (!wait([&] { return !assistant.busy(); }) || assistant.proposals().size() != 1
        || assistant.proposals().first().toMap()["arguments"].toMap()["query"] != prompt) {
        return 6;
    }
    if (!assistant.results().isEmpty() || !assistant.runSearches()
        || assistant.results().size() != 1 || !assistant.proposals().isEmpty()
        || content.songs() != original) {
        return 7;
    }
    for (const auto &prompt : { "malformed", "blocked", "oversize" }) {
        assistant.propose(prompt);
        if (!wait([&] { return !assistant.busy(); }) || !assistant.proposals().isEmpty()
            || !assistant.results().isEmpty()) {
            return 8;
        }
    }
    assistant.propose("scripture", "missing-bible");
    if (!wait([&] { return !assistant.busy(); }) || assistant.runSearches()
        || !assistant.results().isEmpty()) {
        return 9;
    }
    assistant.propose("stall");
    assistant.cancel();
    if (!wait([&] { return !assistant.busy(); }) || !assistant.proposals().isEmpty()) {
        return 10;
    }
    assistant.propose("faith");
    if (!wait([&] { return !assistant.busy(); }) || assistant.proposals().size() != 1) {
        return 11;
    }
    assistant.propose("faith", "", "unsupported");
    if (assistant.busy() || !assistant.proposals().isEmpty()) {
        return 16;
    }
    assistant.propose("scripture", "engwebp", "songs");
    if (!wait([&] { return !assistant.busy(); }) || !assistant.proposals().isEmpty()) {
        return 17;
    }
    assistant.setEnabled(false);
    if (assistant.runSearches() || !assistant.proposals().isEmpty()
        || content.songs() != original) {
        return 12;
    }

    CactusBackend deadline(nullptr, 150);
    QString failure;
    QObject::connect(&deadline, &CactusBackend::completed,
        [&](const QByteArray &, const QString &error) { failure = error; });
    deadline.request(runtime, model, "stall", "test context");
    if (!wait([&] { return !deadline.busy(); }) || !failure.contains("timed out")) {
        return 13;
    }
    deadline.request(directory.filePath("missing-runtime"), model, "faith", "context");
    if (!wait([&] { return !deadline.busy(); }) || failure.isEmpty()) {
        return 14;
    }
    qInfo() << "AI foundation checks passed: strict proposals, isolated worker, cancellation, "
               "timeout, offline configuration and read-only catalog searches.";
    return 0;
}
