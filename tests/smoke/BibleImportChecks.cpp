#include "BibleImportChecks.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "VisualLookup.h"
#include <memory>
#include <QDir>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTimer>

void runBibleImportChecks(ContentLibrary &content, EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(230);
        return;
    }
    QDir().mkpath(folder);
    window->resize(1120, 800);
    const auto before = document.slides();
    const auto snapshot = outputs.snapshotId();
    QMetaObject::invokeMethod(window, "browseLibrary", Q_ARG(QVariant, QVariant("bible")));
    auto dialog = window->findChild<QObject *>("bibleImportDialog");
    if (!dialog) {
        qApp->exit(230);
        return;
    }
    auto wrapped = engine.newQObject(dialog);
    wrapped.property("open").callWithInstance(wrapped);
    auto search = window->findChild<QObject *>("repositoryBibleSearch");
    if (!search) {
        qApp->exit(230);
        return;
    }
    search->setProperty("text", "Telugu");

    struct State {
        int phase = 0;
        QElapsedTimer elapsed;
        bool sawDownload = false, sawIndexing = false, capturedProgress = false;
    };

    auto state = std::make_shared<State>();
    state->elapsed.start();
    auto captureProgress = [window, &engine](const QString &path) {
        // Keep network callbacks responsive while the render thread captures an animated bar.
        const auto grab = window->contentItem()->grabToImage();
        if (grab) {
            QObject::connect(
                grab.data(), &QQuickItemGrabResult::ready, &engine,
                [grab, path] { grab->saveToFile(path); }, Qt::SingleShotConnection);
        }
    };
    QObject::connect(&content, &ContentLibrary::bibleImportProgressChanged, &engine,
        [&, state, window, folder, captureProgress] {
            state->sawDownload |= content.bibleImportPhase() == "downloading";
            state->sawIndexing |= content.bibleImportPhase() == "indexing";
            if (content.bibleImportPhase() == "downloading" && content.bibleDownloadedBytes() > 0
                && !state->capturedProgress) {
                state->capturedProgress = true;
                QTimer::singleShot(0, &engine, [folder, captureProgress] {
                    captureProgress(folder + "/bible-download-progress.png");
                });
            }
        });
    auto timer = new QTimer(&engine);
    timer->setInterval(600);
    QObject::connect(timer, &QTimer::timeout, &engine,
        [&, state, timer, window, dialog, folder, before, snapshot, captureProgress] {
            auto fail = [&](const QString &message) {
                qCritical() << "Bible import check failed:" << message;
                timer->stop();
                qApp->exit(231);
            };
            if (state->elapsed.elapsed() > 60000) {
                fail("Download/import timed out");
                return;
            }
            if (state->phase == 0) {
                auto row
                    = VisualLookup::find(window->contentItem(), "repositoryBible_TeluguBible.xml");
                auto button
                    = VisualLookup::find(window->contentItem(), "importBibleDownloadButton");
                auto list = window->findChild<QObject *>("repositoryBibleList");
                if (!row || !button || !list || list->property("count").toInt() != 5) {
                    fail("Repository picker and Telugu filtering");
                    return;
                }
                QMetaObject::invokeMethod(row, "clicked");
                state->phase = 1;
            } else if (state->phase == 1) {
                auto button
                    = VisualLookup::find(window->contentItem(), "importBibleDownloadButton");
                if (!button || !button->property("enabled").toBool()) {
                    fail("Selecting a translation enables download");
                    return;
                }
                window->grabWindow().save(folder + "/bible-repository-picker.png");
                QMetaObject::invokeMethod(button, "clicked");
                if (!content.busy()) {
                    fail(content.status());
                    return;
                }
                QTimer::singleShot(150, &engine, [&, folder, captureProgress] {
                    if (content.busy()) {
                        captureProgress(folder + "/bible-import-connecting.png");
                    }
                });
                state->phase = 2;
            } else if (state->phase == 2 && !content.busy()) {
                if (content.status() != "Translation imported" || content.translations().size() != 4
                    || !state->sawDownload || !state->sawIndexing
                    || content.bibleDownloadedBytes() <= 0
                    || !content.bibleImportPhase().isEmpty()) {
                    fail(content.status());
                    return;
                }
                auto record = QVariantMap();
                for (const auto &value : content.translations()) {
                    if (value.toMap()["title"] == "Telugu (BSI) 1880") {
                        record = value.toMap();
                    }
                }
                const auto id = record["id"].toString();
                if (id.isEmpty() || content.books(id).size() != 66
                    || content.passage(id, "GEN", 1, 1, 31).size() != 31
                    || content.searchVerses(id, "దేవుడు").isEmpty() || document.slides() != before
                    || outputs.snapshotId() != snapshot || dialog->property("opened").toBool()) {
                    fail("Imported passages, Unicode search, dialog close or live isolation");
                    return;
                }
                state->phase = 3;
            } else if (state->phase == 3) {
                window->grabWindow().save(folder + "/bible-imported-telugu.png");
                timer->stop();
                qInfo()
                    << "Bible import native checks passed: picker, Telugu filter, repository "
                       "download, 66-book import, passage navigation, search and live isolation.";
                qApp->exit(0);
            }
        });
    timer->start();
}
