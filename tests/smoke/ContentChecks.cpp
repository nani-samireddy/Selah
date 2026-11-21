#include "ContentLibrary.h"
#include "DiagnosticRunner.h"
#include "EditorDocument.h"
#include "MediaLibrary.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "ProgramChecks.h"
#include "ProgramStore.h"
#include "PrototypeChecks.h"
#include "SlideFactory.h"
#include "VisualLookup.h"
#include <QDebug>
#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>

int startContentChecks(QGuiApplication &app, EditorDocument &document, MediaLibrary &library,
    ContentLibrary &content, OutputManager &outputs, NdiController &ndi, ProgramStore &programs,
    QQmlApplicationEngine &engine) {
    const int contentIndex = app.arguments().indexOf("--content-smoke");
    if (contentIndex >= 0) {
        if (contentIndex + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
            return 80;
        }
        const auto folder = app.arguments()[contentIndex + 1];
        QDir().mkpath(folder);
        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (app.arguments().contains("--compact")) {
            window->setWidth(1120);
            window->setHeight(760);
        }
        QVariantMap song { { "id", "content-smoke-original" }, { "title", "Together in hope" },
            { "primaryLanguage", "English" }, { "secondaryLanguage", "తెలుగు" },
            { "credits", "Selah · Original verification lyrics" }, { "tags", "Gathering" },
            { "sections",
                QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                                   { "primary",
                                       "We gather here in hope\nWe lift our hearts together\nYour "
                                       "light will guide our way\nThrough every changing season" },
                                   { "secondary",
                                       "ఆశతో ఇక్కడ కూడుదము\nమన హృదయాలను పైకెత్తుదము\nనీ వెలుగు మనకు దారి "
                                       "చూపును\nప్రతి మారుతున్న కాలంలో" } },
                    QVariantMap { { "key", "c1" }, { "label", "Chorus" },
                        { "primary", "Together we will sing\nWith grateful hearts today" },
                        { "secondary", "కలిసి మనము పాడుదము\nఈ రోజు కృతజ్ఞతతో" } } } },
            { "arrangement", QVariantList { "v1", "c1", "v1", "c1" } } };
        const auto id = content.saveSong(song);
        if (id.isEmpty()) {
            return 81;
        }
        QTimer::singleShot(600, &app, [&, window, folder, id] {
            auto panel = window->findChild<QObject *>("songsPanel");
            auto object = engine.newQObject(panel);
            object.property("open").callWithInstance(object);
            object.property("load").callWithInstance(
                object, { engine.toScriptValue(content.song(id)) });
            QTimer::singleShot(300, &app, [&, window, folder, id, object]() mutable {
                window->grabWindow().save(folder + "/songs-library.png");
                // Editing fields must survive section switches and saving a named alternative.
                auto lyrics = VisualLookup::find(window->contentItem(), "primaryLyrics");
                if (!lyrics) {
                    qCritical() << "No first section card editor";
                    app.exit(89);
                    return;
                }
                lyrics->setProperty("text",
                    "Edited line one\nEdited line two\nEdited line three\nEdited line four");
                object.property("selectSection").callWithInstance(object, { QJSValue(1) });
                object.property("selectSection").callWithInstance(object, { QJSValue(0) });
                if (!lyrics->property("text").toString().startsWith("Edited line one")) {
                    app.exit(89);
                    return;
                }
                auto arrangements = QVariantList {
                    QVariantMap { { "name", "Full service" },
                        { "order", QVariantList { "v1", "c1", "v1", "c1" } } },
                    QVariantMap { { "name", "Short service" }, { "order", QVariantList { "c1" } } }
                };
                object.setProperty("arrangements", engine.toScriptValue(arrangements));
                object.property("selectArrangement").callWithInstance(object, { QJSValue(1) });
                if (!object.property("save").callWithInstance(object).toBool()
                    || content.song(id)["arrangement"].toList() != QVariantList { "c1" }) {
                    app.exit(90);
                    return;
                }
                object.property("selectArrangement").callWithInstance(object, { QJSValue(0) });
                object.property("save").callWithInstance(object);
                const auto gathered
                    = object.property("gather").callWithInstance(object).toVariant().toMap();
                if (gathered["sections"].toList().size() != 2
                    || gathered["arrangement"].toList().size() != 4) {
                    app.exit(82);
                    return;
                }
                object.property("close").callWithInstance(object);
                const auto before = document.slides();
                if (!document.addSongSlides(content.song(id), 3)) {
                    app.exit(83);
                    return;
                }
                const auto generated = document.slides();
                document.undo();
                if (document.slides() != before) {
                    app.exit(84);
                    return;
                }
                document.redo();
                if (document.slides() != generated) {
                    app.exit(85);
                    return;
                }
                QTimer::singleShot(300, &app, [&, window, folder] {
                    window->grabWindow().save(folder + "/bilingual-slide.png");
                    auto panel = engine.newQObject(window->findChild<QObject *>("biblePanel"));
                    panel.property("open").callWithInstance(panel);
                    panel.property("openPassage")
                        .callWithInstance(panel,
                            { QJSValue("tel2017"), QJSValue("JHN"), QJSValue(3), QJSValue(16),
                                QJSValue(18) });
                    QTimer::singleShot(350, &app, [&, window, folder, panel]() mutable {
                        if (panel.property("verses").toVariant().toList().size() < 3
                            || panel.property("selected").toVariant().toList().size() != 3) {
                            qCritical() << "Passage UI failed";
                            app.exit(86);
                            return;
                        }
                        window->grabWindow().save(folder + "/bible-library.png");
                        auto search = window->findChild<QObject *>("scriptureSearch");
                        search->setProperty("text", "దేవుడు");
                        QTimer::singleShot(400, &app, [&, window, folder, panel]() mutable {
                            if (panel.property("verses").toVariant().toList().isEmpty()) {
                                app.exit(87);
                                return;
                            }
                            window->grabWindow().save(folder + "/scripture-search.png");
                            panel.property("close").callWithInstance(panel);
                            if (!document.addScriptureSlides(
                                    content.passage("tel2017", "JHN", 3, 16, 18), 4)) {
                                app.exit(88);
                                return;
                            }
                            outputs.takeLive(document.slides(), document.currentSlide());
                            outputs.preview("audience");
                            QTimer::singleShot(350, &app, [&, window, folder] {
                                window->grabWindow().save(folder + "/scripture-slide.png");
                                for (auto output : QGuiApplication::topLevelWindows()) {
                                    if (output->title().contains("Audience output")) {
                                        qobject_cast<QQuickWindow *>(output)->grabWindow().save(
                                            folder + "/scripture-output.png");
                                    }
                                }
                                outputs.stop("audience");
                                qInfo() << "Content smoke passed: native song/Bible panels, "
                                           "Unicode search, slide generation, atomic undo/redo, "
                                           "and live output.";
                                app.exit(0);
                            });
                        });
                    });
                });
            });
        });
    }

    return 0;
}
