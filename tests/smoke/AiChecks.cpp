#include "AiChecks.h"
#include "AiAssistant.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "VisualLookup.h"
#include <memory>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJSValue>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

void runAiChecks(EditorDocument &document, ContentLibrary &content, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder, const QString &runtime,
    const QString &model) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    auto assistant = window
        ? qobject_cast<AiAssistant *>(window->property("assistant").value<QObject *>())
        : nullptr;
    auto programs = window
        ? qobject_cast<ProgramStore *>(window->property("programs").value<QObject *>())
        : nullptr;
    if (!assistant || !programs
        || !assistant->configure(QUrl::fromLocalFile(runtime), QUrl::fromLocalFile(model))) {
        qApp->exit(420);
        return;
    }
    assistant->setEnabled(true);
    for (const auto &title :
        { QString("Grace AI verification"), QString("నీ కృప Luma verification") }) {
        if (content
                .saveSong({ { "title", title }, { "primaryLanguage", "English" },
                    { "sections",
                        QVariantList { QVariantMap {
                            { "key", "v1" }, { "label", "Verse 1" }, { "primary", title } } } },
                    { "arrangement", QVariantList { "v1" } } })
                .isEmpty()) {
            qApp->exit(421);
            return;
        }
    }

    struct Case {
        QString prompt, scope, translation, code, title;
        int chapter = 0, verse = 0;
        bool clarify = false;
    };

    const QList<Case> cases { { "Find songs about grcae", "songs", "engwebp", {},
                                  "Grace AI verification" },
        { "నీ కృప గురించి పాటలను వెతుకు", "songs", "tel2017", {}, "నీ కృప Luma verification" },
        { "నీ కృప songs చూపించు", "songs", "tel2017", {}, "నీ కృప Luma verification" },
        { "genses 1:1", "scripture", "engwebp", "GEN", {}, 1, 1 },
        { "యోహాను 3:16 చూపించు", "scripture", "tel2017", "JHN", {}, 3, 16 },
        { "show ఆదికాండము 1:1", "scripture", "tel2017", "GEN", {}, 1, 1 },
        { "adikandam 1:1", "scripture", "tel2017", "GEN", {}, 1, 1 },
        { "show aadikandamu 1:1 చూపించు", "scripture", "tel2017", "GEN", {}, 1, 1 },
        { "utpatti 1:1", "scripture", "tel2017", "GEN", {}, 1, 1 },
        { "show 3:16", "scripture", "engwebp", {}, {}, 0, 0, true },
        { "John or James 3:16", "scripture", "engwebp", {}, {}, 0, 0, true },
        { "John", "scripture", "engwebp", {}, {}, 0, 0, true },
        { "John 3:99", "scripture", "engwebp", {}, {}, 0, 0, true } };
    const auto draft = document.slides(), catalog = content.songs(), program = programs->items();
    outputs.takeLive(draft, 0, "ai-protected-output");
    const auto snapshot = outputs.snapshotId();
    QDir().mkpath(folder);
    QMetaObject::invokeMethod(window, "showAiAssistant");

    struct State {
        int index = 0, waits = 0;
        bool requested = false;
        QVariantList report;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(200);
    QObject::connect(timer, &QTimer::timeout, &engine,
        [&, window, assistant, programs, cases, state, timer, draft, catalog, program, snapshot,
            folder] {
            auto fail = [&](const QString &reason) {
                qCritical() << "Luma validation failed" << state->index << reason
                            << assistant->status();
                timer->stop();
                assistant->cancel();
                qApp->exit(422);
            };
            if (assistant->busy()) {
                if (++state->waits > 180) {
                    fail("Inference timeout");
                }
                return;
            }
            if (state->index == cases.size()) {
                QFile file(folder + "/evaluation.json");
                if (!file.open(QIODevice::WriteOnly)) {
                    fail("Could not write evaluation");
                    return;
                }
                file.write(QJsonDocument::fromVariant(state->report).toJson());
                window->grabWindow().save(folder + "/luma-evaluation.png");
                qInfo() << "Luma validation passed: English model routing; Telugu and "
                           "mixed-language request parsing; ambiguous reference clarification; "
                           "exact saved results; unchanged live/program/draft.";
                timer->stop();
                auto dialog = window->findChild<QObject *>("aiAssistantDialog");
                QMetaObject::invokeMethod(dialog, "close");
                QMetaObject::invokeMethod(window, "showLibrarySection", Q_ARG(QVariant, "bible"));
                auto browser = window->findChild<QObject *>("biblePanel");
                QMetaObject::invokeMethod(browser, "openPassage", Q_ARG(QVariant, "tel2017"),
                    Q_ARG(QVariant, "GEN"), Q_ARG(QVariant, 1), Q_ARG(QVariant, 1),
                    Q_ARG(QVariant, 1));
                QTimer::singleShot(350, &engine, [&, window, browser, folder] {
                    auto field = VisualLookup::find(
                        browser->property("contentItem").value<QQuickItem *>(), "scriptureSearch");
                    if (!field) {
                        qApp->exit(423);
                        return;
                    }
                    field->setProperty("text", "prakatana 2:3");
                    QTimer::singleShot(450, &engine, [&, window, browser, folder] {
                        auto value = browser->property("verses");
                        if (value.metaType() == QMetaType::fromType<QJSValue>()) {
                            value = value.value<QJSValue>().toVariant();
                        }
                        if (value.toList() != content.passage("tel2017", "REV", 2, 3, 3)
                            || !browser->property("message").toString().isEmpty()) {
                            qCritical() << "Romanized Bible search UI failed"
                                        << browser->property("message");
                            qApp->exit(424);
                            return;
                        }
                        window->grabWindow().save(folder + "/romanized-bible-search.png");
                        qInfo() << "prakatana 2:3 verified in the Telugu Bible search interface.";
                        QMetaObject::invokeMethod(browser, "choose", Q_ARG(QVariant, 0));
                        auto map = [](QVariant value) {
                            if (value.metaType() == QMetaType::fromType<QJSValue>()) {
                                value = value.value<QJSValue>().toVariant();
                            }
                            return value.toMap();
                        };
                        if (map(browser->property("book"))["code"] != "REV"
                            || map(browser->property("chapter"))["chapter"].toInt() != 2
                            || browser->property("highlightedVerse").toInt() != 3
                            || !browser->property("searchMode").toBool()) {
                            qCritical() << "Search selection did not synchronize the sidebar";
                            qApp->exit(426);
                            return;
                        }

                        QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
                        QCoreApplication::sendEvent(window, &right);
                        QTimer::singleShot(250, &engine, [browser] {
                            if (browser->property("highlightedVerse").toInt() != 4) {
                                qCritical()
                                    << "Reference search did not continue to the next verse";
                                qApp->exit(425);
                                return;
                            }
                            qInfo() << "Right arrow continued from a one-verse reference search "
                                       "into the chapter.";
                            qApp->exit(0);
                        });
                    });
                });
                return;
            }
            const auto test = cases[state->index];
            if (!state->requested) {
                assistant->propose(test.prompt, test.translation, test.scope);
                state->requested = true;
                state->waits = 0;
                return;
            }
            const auto proposal = assistant->proposals();
            if (test.clarify) {
                if (!proposal.isEmpty() || !assistant->results().isEmpty()) {
                    fail("Ambiguous/unavailable reference was proposed");
                    return;
                }
            } else {
                if (proposal.isEmpty() || !assistant->results().isEmpty()) {
                    fail("A proposal must await approval");
                    return;
                }
                if (!assistant->runSearches()) {
                    fail("Approval failed");
                    return;
                }
                QVariantList rows;
                for (const auto &group : assistant->results()) {
                    rows += group.toMap()["items"].toList();
                }
                if (test.scope == "scripture") {
                    const auto expected = content.passage(
                        test.translation, test.code, test.chapter, test.verse, test.verse);
                    if (expected.isEmpty() || rows != expected) {
                        fail("Reference differs from exact catalog text");
                        return;
                    }
                } else {
                    bool found = false;
                    for (const auto &row : rows) {
                        found |= row.toMap()["title"] == test.title;
                    }
                    if (!found) {
                        fail("Saved song missing");
                        return;
                    }
                }
            }
            if (document.slides() != draft || content.songs() != catalog
                || programs->items() != program || outputs.snapshotId() != snapshot) {
                fail("Read-only state changed");
                return;
            }
            state->report.append(QVariantMap { { "prompt", test.prompt }, { "scope", test.scope },
                { "passed", true },
                { "route",
                    test.clarify ? "clarification"
                                 : proposal.first().toMap().value("origin", "model").toString() },
                { "status", assistant->status() } });
            qInfo() << "Luma case passed" << test.prompt << assistant->status();
            ++state->index;
            state->requested = false;
        });
    timer->start();
}
