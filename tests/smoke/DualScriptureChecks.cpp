#include "DualScriptureChecks.h"
#include "ContentLibrary.h"
#include "ProgramStore.h"
#include "VisualLookup.h"
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTimer>

void runDualScriptureUiChecks(ContentLibrary &content, ProgramStore &programs,
    QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(240);
        return;
    }
    QDir().mkpath(folder);
    window->resize(1280, 900);
    QMetaObject::invokeMethod(window, "browseLibrary", Q_ARG(QVariant, QVariant("bible")));
    QTimer::singleShot(800, &engine, [&, window, folder] {
        auto primary = VisualLookup::find(window->contentItem(), "bibleVersionPicker");
        auto second = VisualLookup::find(window->contentItem(), "bibleSecondaryPicker");
        auto panel = window->findChild<QObject *>("biblePanel");
        auto help = window->findChild<QObject *>("userGuideDialog");
        if (!primary || !second || !panel || !help) {
            qCritical() << "Dual Bible UI controls missing";
            qApp->exit(241);
            return;
        }
        // Invoke the same QML handlers used by translation selection and verse navigation.
        auto wrapped = engine.newQObject(panel);
        wrapped.property("openPassage").callWithInstance(wrapped, { "engwebp", "JHN", 3, 16, 16 });
        QTimer::singleShot(500, &engine, [&, window, second, panel, help, folder] {
            const auto records
                = panel->property("secondaryVersions").value<QJSValue>().toVariant().toList();
            int index = -1;
            for (int i = 0; i < records.size(); ++i) {
                if (records[i].toMap()["id"] == "tel2017") {
                    index = i;
                }
            }
            if (index < 0) {
                qApp->exit(242);
                return;
            }
            second->setProperty("currentIndex", index);
            QMetaObject::invokeMethod(second, "activated", Q_ARG(int, index));
            QTimer::singleShot(500, &engine, [&, window, panel, help, folder] {
                const auto rawSelection = panel->property("pairedSelection");
                const auto selection = rawSelection.metaType() == QMetaType::fromType<QJSValue>()
                    ? rawSelection.value<QJSValue>().toVariant().toMap()
                    : rawSelection.toMap();
                const auto verses = selection["verses"].toList();
                if (!selection["error"].toString().isEmpty() || verses.size() != 1
                    || verses.first().toMap()["verse"].toInt() != 16
                    || verses.first().toMap()["secondary"].toMap()["translation"] != "tel2017"
                    || !programs.createProgram("Dual scripture verification", "2026-11-22")
                    || !programs.addScripture(verses, 4)) {
                    qCritical() << "Dual Bible selection failed" << selection << programs.status();
                    qApp->exit(243);
                    return;
                }
                const auto grab = window->contentItem()->grabToImage();
                QObject::connect(
                    grab.data(), &QQuickItemGrabResult::ready, &engine,
                    [grab, help, folder, &engine, window] {
                        grab->saveToFile(folder + "/dual-bible-library.png");
                        auto helpObject = engine.newQObject(help);
                        helpObject.property("open").callWithInstance(helpObject);
                        QTimer::singleShot(500, &engine, [window, help, folder, &engine] {
                            if (!help->property("opened").toBool()) {
                                qApp->exit(244);
                                return;
                            }
                            const auto helpGrab = window->contentItem()->grabToImage();
                            QObject::connect(
                                helpGrab.data(), &QQuickItemGrabResult::ready, &engine,
                                [helpGrab, folder] {
                                    helpGrab->saveToFile(folder + "/user-guide.png");
                                    qInfo() << "Dual Bible and offline user guide UI checks passed";
                                    qApp->exit(0);
                                },
                                Qt::SingleShotConnection);
                        });
                    },
                    Qt::SingleShotConnection);
            });
        });
    });
    QTimer::singleShot(20000, &engine, [] { qApp->exit(245); });
    Q_UNUSED(content);
}
