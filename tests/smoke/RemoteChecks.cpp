#include "RemoteChecks.h"
#include "DocumentElements.h"
#include "OutputManager.h"
#include "RemoteController.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

void startRemoteDemo(RemoteController &remote, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder) {
    QDir().mkpath(folder);
    QVariantList slides;
    for (int i = 0; i < 4; ++i) {
        auto primary = DocumentElements::text("Primary",
            i == 0       ? "నీ కృప నన్ను నడిపించును\nప్రతి రోజూ నీతో సాగెదను"
                : i == 1 ? "Your love will lead us\nThrough every season"
                : i == 2 ? "यीशु तेरा धन्यवाद\nतेरी महिमा सदा रहे"
                         : "Thank you for joining us",
            140, 220, 1640, 500, 100, "Noto Sans Telugu", "#FFFFFF");
        primary["binding"] = "Primary lyrics";
        primary["alignment"] = "center";
        auto secondary = DocumentElements::text("Translation",
            i == 0 ? "Your grace will guide me\nEvery day I walk with You" : "", 140, 760, 1640,
            220, 56, "Arial", "#C0C8E0");
        secondary["binding"] = "Translated lyrics";
        secondary["alignment"] = "center";
        slides.append(QVariantMap {
            { "name", QStringList { "Verse 1", "Chorus", "Hindi welcome", "Closing" }[i] },
            { "background", i == 2 ? "#384436" : "#323648" }, { "ambientOpacity", 0.4 },
            { "notes",
                i == 0 ? "Repeat softly, then invite the congregation to sing."
                       : "Pause before the next section." },
            { "elements", QVariantList { primary, secondary } },
            { "programTitle", "Sunday Worship" }, { "programItemId", i < 2 ? "song" : "welcome" },
            { "programItemTitle",
                i < 2 ? "Your grace · Telugu / English" : "Welcome & closing" } });
    }
    outputs.takeLive(slides, 0, "remote-verification");
    outputs.setStageMessage("Welcome the congregation");
    outputs.startCountdown(5);
    if (!remote.start("127.0.0.1")) {
        return;
    }
    QFile connection(QDir(folder).filePath("connection.json"));
    if (connection.open(QIODevice::WriteOnly)) {
        connection.write(
            QJsonDocument::fromVariant(QVariantMap { { "url", remote.address() },
                                           { "controllerCode", remote.controllerCode() },
                                           { "speakerCode", remote.speakerCode() } })
                .toJson());
    }
    QFile qr(QDir(folder).filePath("pairing.png"));
    if (qr.open(QIODevice::WriteOnly)) {
        qr.write(QByteArray::fromBase64(remote.controllerQr().section(',', 1).toLatin1()));
    }
    QTimer::singleShot(1000, &engine, [&engine, folder] {
        auto root = engine.rootObjects().value(0);
        auto panel = root ? root->findChild<QObject *>("remotePanel") : nullptr;
        if (panel) {
            QMetaObject::invokeMethod(panel, "open");
        }
        QTimer::singleShot(800, &engine, [&engine, folder] {
            auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
            if (window) {
                window->grabWindow().save(QDir(folder).filePath("desktop.png"));
            }
        });
    });
}
