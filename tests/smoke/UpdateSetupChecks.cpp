#include "UpdateSetupChecks.h"
#include "AiAssistant.h"
#include "ModelInstaller.h"
#include "UpdateManager.h"
#include "VerifiedDownload.h"
#include <memory>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

void runUpdateSetupChecks(QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(580);
        return;
    }
    auto updater = qobject_cast<UpdateManager *>(window->property("updater").value<QObject *>());
    auto installer
        = qobject_cast<ModelInstaller *>(window->property("modelInstaller").value<QObject *>());
    auto assistant = qobject_cast<AiAssistant *>(window->property("assistant").value<QObject *>());
    if (!updater || !installer || !assistant) {
        qApp->exit(581);
        return;
    }
    assistant->setEnabled(false);
    QDir().mkpath(folder);
    QMetaObject::invokeMethod(window, "showUpdates");

    struct State {
        int ticks = 0;
        bool downloading = false, captured = false, integrityChecked = false;
        int probePhase = 0;
    };

    auto state = std::make_shared<State>();
    // A checksum failure and cancellation must both preserve an existing destination.
    const auto probePath = folder + "/download-preservation-test.bin";
    QFile existing(probePath);
    if (!existing.open(QIODevice::WriteOnly) || existing.write("keep existing file") < 0) {
        qApp->exit(585);
        return;
    }
    existing.close();
    const auto preservedHash
        = QCryptographicHash::hash("keep existing file", QCryptographicHash::Sha256).toHex();
    auto probe = new VerifiedDownload(&engine);
    QObject::connect(probe, &VerifiedDownload::progress, &engine, [=](qint64 received, qint64) {
        if (state->probePhase == 1 && received > 0) {
            probe->cancel();
        }
    });
    QObject::connect(probe, &VerifiedDownload::finished, &engine,
        [=](const QString &path, const QString &error) {
            if (!path.isEmpty() || error.isEmpty()
                || !VerifiedDownload::matches(probePath, preservedHash)) {
                qCritical() << "Unverified or cancelled download replaced a destination" << error;
                qApp->exit(586);
                return;
            }
            if (state->probePhase == 0) {
                if (!error.contains("Checksum")) {
                    qCritical() << error;
                    qApp->exit(587);
                    return;
                }
                state->probePhase = 1;
                probe->start(QUrl("https://huggingface.co/Cactus-Compute/needle3/resolve/"
                                  "c95d25b4efa1e9601e785d92438e4ade0dbddbf8/needle3.cact"),
                    probePath, "c9d915eca282ed42d1a09b143b592adb4cc6744ffe2d294adf5cfc5548170c38",
                    100 * 1024 * 1024);
            } else {
                if (!error.contains("cancelled")) {
                    qCritical() << error;
                    qApp->exit(588);
                    return;
                }
                state->integrityChecked = true;
                qInfo() << "Verified HTTPS download checksum failure and cancellation preserved "
                           "existing files.";
            }
        });
    probe->start(QUrl("https://huggingface.co/Cactus-Compute/needle3/resolve/"
                      "c95d25b4efa1e9601e785d92438e4ade0dbddbf8/LICENSE"),
        probePath, QByteArray(64, '0'), 100 * 1024);
    auto timer = new QTimer(&engine);
    timer->setInterval(200);
    QObject::connect(timer, &QTimer::timeout, &engine, [=] {
        if (++state->ticks > 900) {
            installer->cancel();
            qCritical() << "Setup timeout" << installer->status();
            qApp->exit(582);
            return;
        }
        if (!state->downloading) {
            if (updater->busy()) {
                return;
            }
            if (!window->grabWindow().save(folder + "/updates.png")) {
                qApp->exit(583);
                return;
            }
            qInfo() << "Update check:" << updater->status();
            auto dialog = window->findChild<QObject *>("updateDialog");
            QMetaObject::invokeMethod(dialog, "close");
            QMetaObject::invokeMethod(window, "showAiAssistant");
            auto aiDialog = window->findChild<QObject *>("aiAssistantDialog");
            aiDialog->setProperty("setupOpen", true);
            state->downloading = true;
            installer->install();
            return;
        }
        if (!state->integrityChecked && !installer->busy()) {
            return;
        }
        if (installer->busy()) {
            if (!state->captured && installer->received() > 0) {
                state->captured = window->grabWindow().save(folder + "/luma-download.png");
            }
            return;
        }
        if (!ModelInstaller::managedReady() || !assistant->configured() || assistant->enabled()) {
            qCritical() << "Managed setup failed" << installer->status() << assistant->status();
            qApp->exit(584);
            return;
        }
        window->grabWindow().save(folder + "/luma-ready.png");
        qInfo()
            << "Update and model setup UI passed. Managed files verified; inference remains off.";
        timer->stop();
        qApp->exit(0);
    });
    timer->start();
}
