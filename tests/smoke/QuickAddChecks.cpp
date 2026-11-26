#include "QuickAddChecks.h"
#include "OutputManager.h"
#include "QuickAddController.h"
#include "VisualLookup.h"
#include <QDir>
#include <QGuiApplication>
#include <QJSValue>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

void runQuickAddUiChecks(QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(470);
        return;
    }
    auto quick
        = qobject_cast<QuickAddController *>(window->property("quickAdd").value<QObject *>());
    auto outputs = qobject_cast<OutputManager *>(window->property("outputs").value<QObject *>());
    if (!quick || !outputs) {
        qApp->exit(471);
        return;
    }
    window->setProperty("activeWorkspace", "present");
    outputs->takeLive(
        { QVariantMap { { "name", "Original slide" }, { "programItemId", "original" },
            { "programItemTitle", "Original program item" },
            { "elements",
                QVariantList { QVariantMap { { "kind", "text" },
                    { "text", "Original live content" }, { "fontSize", 96 }, { "visible", true },
                    { "x", 160 }, { "y", 200 }, { "width", 1600 }, { "height", 400 } } } } } },
        0, "quick-ui-test");
    QDir().mkpath(folder);
    QTimer::singleShot(500, &engine, [=, &engine] {
        auto open = VisualLookup::find(window->contentItem(), "quickAddOpenButton");
        if (!open) {
            qApp->exit(472);
            return;
        }
        QMetaObject::invokeMethod(open, "clicked");
        QTimer::singleShot(350, &engine, [=, &engine] {
            auto dialog = window->findChild<QObject *>("quickAddDialog");
            if (!dialog || !dialog->property("opened").toBool()) {
                qApp->exit(473);
                return;
            }
            dialog->setProperty("section", "bible");
            auto reference = VisualLookup::find(window->contentItem(), "quickAddReference");
            auto preview = VisualLookup::find(window->contentItem(), "quickAddPassagePreview");
            reference->setProperty("text", "genses 1:1");
            QMetaObject::invokeMethod(preview, "clicked");
            QTimer::singleShot(400, &engine, [=, &engine] {
                if (quick->slides().isEmpty() || outputs->liveText() != "Original live content") {
                    qCritical() << "Quick Add preview check failed" << quick->status();
                    qApp->exit(474);
                    return;
                }
                if (!window->grabWindow().save(folder + "/quick-bible.png")) {
                    qApp->exit(475);
                    return;
                }
                auto now = VisualLookup::find(window->contentItem(), "quickAddPresentNow");
                QMetaObject::invokeMethod(now, "clicked");
                QTimer::singleShot(350, &engine, [=, &engine] {
                    if (!outputs->canResumeProgram() || outputs->liveTitle() == "Original slide") {
                        qApp->exit(476);
                        return;
                    }
                    window->grabWindow().save(folder + "/quick-live.png");
                    auto resume = VisualLookup::find(window->contentItem(), "resumeProgramButton");
                    QMetaObject::invokeMethod(resume, "clicked");
                    if (outputs->liveText() != "Original live content") {
                        qApp->exit(477);
                        return;
                    }
                    QMetaObject::invokeMethod(open, "clicked");
                    dialog->setProperty("section", "quickSong");
                    quick->setDraft({ { "title", "Unexpected song" },
                        { "text", "We sing together\n\nA new song of praise" } });
                    auto draftPreview
                        = VisualLookup::find(window->contentItem(), "quickAddDraftPreview");
                    QMetaObject::invokeMethod(draftPreview, "clicked");
                    QTimer::singleShot(350, &engine, [=, &engine] {
                        if (quick->slides().size() != 2 || !quick->canSaveDraft()) {
                            qApp->exit(478);
                            return;
                        }
                        window->grabWindow().save(folder + "/quick-song.png");
                        auto insert
                            = VisualLookup::find(window->contentItem(), "quickAddInsertNext");
                        QMetaObject::invokeMethod(insert, "clicked");
                        if (outputs->liveText() != "Original live content"
                            || outputs->liveCount() < 4) {
                            qApp->exit(479);
                            return;
                        }
                        window->setProperty("activeWorkspace", "design");
                        auto toggle
                            = VisualLookup::find(window->contentItem(), "toggleEditorSlidesPanel");
                        QMetaObject::invokeMethod(toggle, "clicked");
                        QTimer::singleShot(500, &engine, [=, &engine] {
                            auto dock
                                = VisualLookup::find(window->contentItem(), "editorFloatingDock");
                            auto compact
                                = VisualLookup::find(window->contentItem(), "compactEditorSlides");
                            if (!dock || !compact || !compact->isVisible()
                                || compact->y() < dock->y() + dock->height()) {
                                qApp->exit(480);
                                return;
                            }
                            window->grabWindow().save(folder + "/minimized-slides.png");
                            qInfo() << "Quick Add and editor UI checks passed: preview, spelling, "
                                       "present now, resume, insert next and slides below toolbar.";
                            window->setProperty("activeWorkspace", "present");
                            auto scriptureOpen = VisualLookup::find(
                                window->contentItem(), "scriptureSessionOpenButton");
                            QMetaObject::invokeMethod(scriptureOpen, "clicked");
                            QTimer::singleShot(350, &engine, [=, &engine] {
                                auto browser
                                    = window->findChild<QObject *>("scriptureSessionBible");
                                if (!browser || !browser->property("opened").toBool()) {
                                    qApp->exit(481);
                                    return;
                                }
                                QMetaObject::invokeMethod(browser, "openPassage",
                                    Q_ARG(QVariant, "engwebp"), Q_ARG(QVariant, "JHN"),
                                    Q_ARG(QVariant, 3), Q_ARG(QVariant, 16), Q_ARG(QVariant, 16));
                                QTimer::singleShot(400, &engine, [=, &engine] {
                                    if (quick->slides().isEmpty()
                                        || outputs->liveText() != "Original live content") {
                                        qCritical()
                                            << "Scripture preview failed" << quick->status();
                                        qApp->exit(482);
                                        return;
                                    }
                                    auto arrow = [window](int key) {
                                        QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
                                        QCoreApplication::sendEvent(window, &press);
                                        QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
                                        QCoreApplication::sendEvent(window, &release);
                                    };
                                    auto selectedVerse = [browser] {
                                        auto list = [](QVariant value) {
                                            if (value.metaType()
                                                == QMetaType::fromType<QJSValue>()) {
                                                value = value.value<QJSValue>().toVariant();
                                            }
                                            return value.toList();
                                        };
                                        const auto selected = list(browser->property("selected"));
                                        const auto verses = list(browser->property("verses"));
                                        return selected.isEmpty()
                                            ? -1
                                            : verses.value(selected.first().toInt())
                                                  .toMap()["verse"]
                                                  .toInt();
                                    };
                                    QMetaObject::invokeMethod(
                                        browser, "choose", Q_ARG(QVariant, 15));
                                    QMetaObject::invokeMethod(
                                        browser, "choose", Q_ARG(QVariant, 15));
                                    arrow(Qt::Key_Right);
                                    if (selectedVerse() != 17
                                        || outputs->liveText() != "Original live content") {
                                        qCritical() << "Scripture arrow navigation failed"
                                                    << selectedVerse();
                                        qApp->exit(486);
                                        return;
                                    }
                                    arrow(Qt::Key_Left);
                                    arrow(Qt::Key_Up);
                                    if (selectedVerse() != 15) {
                                        qApp->exit(487);
                                        return;
                                    }
                                    arrow(Qt::Key_Down);
                                    if (selectedVerse() != 16) {
                                        qApp->exit(488);
                                        return;
                                    }
                                    QMetaObject::invokeMethod(browser, "presentSelection");
                                    const int count = outputs->liveCount();
                                    QMetaObject::invokeMethod(browser, "presentSelection");
                                    if (outputs->liveCount() != count
                                        || !outputs->canResumeProgram()) {
                                        qApp->exit(483);
                                        return;
                                    }
                                    QMetaObject::invokeMethod(
                                        browser, "continueReading", Q_ARG(QVariant, 1));
                                    QTimer::singleShot(400, &engine, [=, &engine] {
                                        if (!outputs->liveTitle().contains("3:17")
                                            || !browser->property("opened").toBool()
                                            || !window->grabWindow().save(
                                                folder + "/scripture-session.png")) {
                                            qCritical() << "Scripture continuation failed"
                                                        << outputs->liveTitle()
                                                        << browser->property("message");
                                            qApp->exit(484);
                                            return;
                                        }
                                        if (!outputs->resumeProgram()
                                            || outputs->liveText() != "Original live content") {
                                            qApp->exit(485);
                                            return;
                                        }
                                        qInfo() << "Scripture session UI passed: browse, present, "
                                                   "deduplicate, continue, resume.";
                                        QMetaObject::invokeMethod(browser, "sessionClosed");
                                        QMetaObject::invokeMethod(
                                            window, "showLibrarySection", Q_ARG(QVariant, "bible"));
                                        auto libraryBrowser
                                            = window->findChild<QObject *>("biblePanel");
                                        QMetaObject::invokeMethod(libraryBrowser, "openPassage",
                                            Q_ARG(QVariant, "engwebp"), Q_ARG(QVariant, "JHN"),
                                            Q_ARG(QVariant, 3), Q_ARG(QVariant, 16),
                                            Q_ARG(QVariant, 16));
                                        QTimer::singleShot(350, &engine, [=] {
                                            if (!libraryBrowser
                                                || !libraryBrowser->property("opened").toBool()) {
                                                qApp->exit(489);
                                                return;
                                            }
                                            QMetaObject::invokeMethod(
                                                libraryBrowser, "choose", Q_ARG(QVariant, 15));
                                            QMetaObject::invokeMethod(
                                                libraryBrowser, "choose", Q_ARG(QVariant, 15));
                                            QKeyEvent right(
                                                QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
                                            QCoreApplication::sendEvent(window, &right);
                                            if (libraryBrowser->property("highlightedVerse").toInt()
                                                    != 17
                                                || outputs->liveText() != "Original live content") {
                                                qCritical()
                                                    << "Bible library card keyboard navigation "
                                                       "failed"
                                                    << libraryBrowser->property("highlightedVerse");
                                                qApp->exit(490);
                                                return;
                                            }
                                            qInfo() << "Bible card selection and real arrow-key "
                                                       "events passed in both browsers.";
                                            qApp->exit(0);
                                        });
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    });
}
