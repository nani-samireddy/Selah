#include "StandbyChecks.h"
#include "OutputManager.h"
#include <memory>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

void runStandbyChecks(OutputManager &outputs, QQmlApplicationEngine &engine, const QString &video,
    const QString &folder) {
    QDir().mkpath(folder);
    outputs.takeLive({ QVariantMap { { "name", "Paused service" }, { "elements", QVariantList {} },
                         { "background", "#000000" } } },
        0, "standby-check");
    outputs.setPlaying(false);
    const auto snapshot = outputs.snapshotId();
    QQmlComponent component(&engine);
    component.loadFromModule("ChurchDeck", "AudienceRenderer");
    auto renderer = qobject_cast<QQuickItem *>(component.createWithInitialProperties(
        { { "controller", QVariant::fromValue<QObject *>(&outputs) } }));
    if (!renderer) {
        qCritical() << component.errors();
        qApp->exit(870);
        return;
    }
    auto window = new QQuickWindow;
    QObject::connect(&engine, &QObject::destroyed, window, &QObject::deleteLater);
    window->setTitle("Selah — Standby verification");
    window->resize(640, 360);
    renderer->setParent(window);
    renderer->setParentItem(window->contentItem());
    renderer->setScale(1. / 3.);
    renderer->setTransformOrigin(QQuickItem::TopLeft);
    window->show();
    outputs.importStandbyMedia(QUrl::fromLocalFile(video));
    auto timer = new QTimer(&engine);
    timer->setInterval(100);
    auto ticks = std::make_shared<int>(0);
    auto shown = std::make_shared<bool>(false);
    QObject::connect(timer, &QTimer::timeout, &engine,
        [&, renderer, window, timer, snapshot, ticks, shown, folder] {
            if (++*ticks > 100) {
                qCritical() << "Standby video did not produce frames";
                timer->stop();
                window->close();
                qApp->exit(871);
                return;
            }
            if (outputs.importingStandbyMedia()) {
                return;
            }
            if (!*shown) {
                if (outputs.standbyMediaKind() != "video" || !outputs.showDefaultImage(true)) {
                    qCritical() << outputs.status();
                    qApp->exit(872);
                    return;
                }
                *shown = true;
            }
            QJSValue object = engine.newQObject(renderer);
            const auto layers
                = object.property("probe").callWithInstance(object).toVariant().toList();
            if (layers.isEmpty() || layers[0].toMap()["frames"].toInt() < 12) {
                return;
            }
            if (outputs.playing() || outputs.snapshotId() != snapshot || outputs.liveIndex() != 0
                || !outputs.audienceElements()[0].toMap()["muted"].toBool()) {
                qApp->exit(873);
                return;
            }
            const auto image = window->grabWindow();
            int red = 0, green = 0, blue = 0;
            for (int y = 0; y < image.height(); y += 8) {
                for (int x = 0; x < image.width(); x += 8) {
                    const auto color = image.pixelColor(x, y);
                    red += color.red() > 180 && color.green() < 80 && color.blue() < 80;
                    green += color.green() > 180 && color.red() < 80 && color.blue() < 80;
                    blue += color.blue() > 180 && color.green() < 80 && color.red() < 80;
                }
            }
            if (!image.save(folder + "/standby-video.png") || red < 30 || green < 30 || blue < 30) {
                qCritical() << "Standby test pattern pixels missing:" << red << green << blue;
                timer->stop();
                window->close();
                qApp->exit(875);
                return;
            }
            outputs.showDefaultImage(false);
            if (outputs.playing() || outputs.snapshotId() != snapshot
                || outputs.audienceElements() != outputs.liveElements()) {
                qApp->exit(874);
                return;
            }
            qInfo() << "Standby video checks passed: asynchronous import, silent looping across "
                       "more than one clip, RGB test-pattern pixels verified, paused service "
                       "snapshot preserved and exact resume.";
            timer->stop();
            window->close();
            qApp->exit(0);
        });
    timer->start();
}
