#include "BackgroundChecks.h"
#include "DocumentElements.h"
#include "EditorDocument.h"
#include "NdiController.h"
#include "OutputManager.h"
#include "TemplateComposer.h"
#include "TemplateStore.h"
#include "VisualLookup.h"
#include <memory>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

void runBackgroundChecks(EditorDocument &document, TemplateStore &templates, OutputManager &outputs,
    NdiController &ndi, QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(320);
        return;
    }
    window->resize(1440, 950);
    QDir().mkpath(folder);
    const auto id = templates.createTemplate("Canvas background test", "song");
    QMetaObject::invokeMethod(
        window, "editTemplate", Q_ARG(QVariant, QVariant(templates.layout(id))));
    auto slide = document.slides().first().toMap();
    auto layers = slide["elements"].toList();
    QFile file(QDir(folder).absoluteFilePath("green.svg"));
    if (!file.open(QIODevice::WriteOnly)) {
        qApp->exit(321);
        return;
    }
    file.write("<svg xmlns='http://www.w3.org/2000/svg' width='200' height='200'><rect width='200' "
               "height='200' fill='#00ff00'/></svg>");
    file.close();
    auto image = DocumentElements::text("Visible artwork", "", 100, 100, 200, 200, 36, "Arial");
    image["kind"] = "image";
    image["source"] = QUrl::fromLocalFile(file.fileName()).toString();
    image["fit"] = "contain";
    layers.append(image);
    slide["elements"] = layers;
    document.replaceSlides({ slide });
    outputs.takeLive(document.slides(), 0, "background-baseline");
    const auto baseline = outputs.snapshotId();

    struct State {
        int phase = 0;
        int captureWaits = 0;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(700);
    QObject::connect(
        timer, &QTimer::timeout, &engine, [&, state, timer, window, id, baseline, layers, folder] {
            auto fail = [&](const QString &message) {
                qCritical() << "Canvas background check failed:" << message;
                timer->stop();
                ndi.stop();
                qApp->exit(322);
            };
            auto find = [&](const QString &name) {
                return VisualLookup::find(window->contentItem(), name);
            };
            if (state->phase == 0) {
                document.setSelected(1);
                auto backgroundButton = find("canvasBackgroundButton");
                if (!backgroundButton || !QMetaObject::invokeMethod(backgroundButton, "clicked")
                    || document.selected() != -1) {
                    fail("Background action must select the canvas settings");
                    return;
                }
                auto color = find("canvasBackgroundColor");
                auto mode = find("canvasBackgroundMode");
                auto pattern = find("canvasAmbientToggle");
                if (!color || !mode || !pattern || !color->isVisible()) {
                    fail("Canvas controls must be visible in the template editor");
                    return;
                }
                color->setProperty("text", "#3A86FF");
                QMetaObject::invokeMethod(color, "editingFinished");
                if (QColor(document.background()) != QColor("#3A86FF")) {
                    fail("Color field must edit the canvas");
                    return;
                }
                QMetaObject::invokeMethod(pattern, "clicked");
                QMetaObject::invokeMethod(mode, "chosen", Q_ARG(QString, QString("transparent")));
                if (QColor(document.background()).alpha() != 0 || document.ambientOpacity() != 0.5
                    || document.elements() != layers || outputs.snapshotId() != baseline) {
                    fail("Transparency must preserve layers and preparation isolation");
                    return;
                }
                if (templates
                        .saveTemplate(
                            "Canvas background test", "song", document.slides().first().toMap(), id)
                        .isEmpty()) {
                    fail("Saving transparent template");
                    return;
                }
                QString error;
                const QVariantMap song { { "title", "Transparent worship" },
                    { "sections",
                        QVariantList {
                            QVariantMap { { "key", "v1" }, { "primary", "Visible lyrics" } } } },
                    { "arrangement", QVariantList { "v1" } } };
                const auto pages
                    = TemplateComposer::songSlides(song, 3, templates.layout(id), &error);
                if (!error.isEmpty() || pages.isEmpty()) {
                    fail(error);
                    return;
                }
                outputs.takeLive(pages, 0);
                if (!ndi.start("Canvas alpha verification", 1280, 720, 30, false, true)) {
                    fail(ndi.status());
                    return;
                }
            } else if (state->phase == 1) {
                const auto frame = ndi.lastFrame();
                // Render-control startup is asynchronous; assert pixels only
                // after its first frame, with a bounded deadline for failures.
                if (frame.isNull() && ++state->captureWaits < 20) {
                    return;
                }
                if (frame.isNull() || frame.pixelColor(10, 10).alpha() != 0
                    || frame.pixelColor(100, 100) != QColor("#00FF00")) {
                    qWarning() << "Alpha capture" << frame.size() << ndi.status();
                    if (!frame.isNull()) {
                        qWarning() << frame.pixelColor(10, 10) << frame.pixelColor(100, 100);
                        frame.save(folder + "/alpha-failure.png");
                    }
                    fail("Transparent template must remove canvas and ambient fill but keep "
                         "artwork");
                    return;
                }
                frame.save(folder + "/transparent-output.png");
                window->grabWindow().save(folder + "/transparent-editor.png");
                auto mode = find("canvasBackgroundMode");
                QMetaObject::invokeMethod(mode, "chosen", Q_ARG(QString, QString("color")));
                if (QColor(document.background()) != QColor("#3A86FF")) {
                    fail("Color mode must restore the previous fill");
                    return;
                }
                document.setBackground("#803A86FF");
                outputs.takeLive(document.slides(), 0);
            } else if (state->phase == 2) {
                const auto frame = ndi.lastFrame();
                if (frame.isNull() || qAbs(frame.pixelColor(10, 10).alpha() - 128) > 2) {
                    fail("Ambient pattern must share the background's alpha");
                    return;
                }
                frame.save(folder + "/partial-alpha-output.png");
                auto pattern = find("canvasAmbientToggle");
                QMetaObject::invokeMethod(pattern, "clicked");
                document.setBackground("#3A86FF");
                outputs.takeLive(document.slides(), 0);
            } else if (state->phase == 3) {
                const auto frame = ndi.lastFrame();
                if (frame.isNull() || frame.pixelColor(10, 10) != QColor("#3A86FF")) {
                    fail("Solid fill must render the chosen color without a fixed pattern");
                    return;
                }
                window->grabWindow().save(folder + "/solid-editor.png");
                const QVariantMap gradient { { "type", "linear" }, { "angle", 0 },
                    { "stops",
                        QVariantList { QVariantMap { { "color", "#FF0000" }, { "position", 0.0 } },
                            QVariantMap { { "color", "#0000FF" }, { "position", 1.0 } } } } };
                document.setBackgroundFill(gradient);
                outputs.takeLive(document.slides(), 0);
            } else if (state->phase == 4) {
                const auto frame = ndi.lastFrame();
                if (frame.isNull() || frame.pixelColor(5, 360).red() < 245
                    || frame.pixelColor(1275, 360).blue() < 245) {
                    fail("Linear gradient must reach both color stops");
                    return;
                }
                frame.save(folder + "/linear-output.png");
                window->grabWindow().save(folder + "/gradient-editor.png");
                const auto previousStops = document.backgroundFill().value("stops");
                QMetaObject::invokeMethod(
                    find("canvasBackgroundMode"), "chosen", Q_ARG(QString, QString("radial")));
                if (document.backgroundFill().value("stops") != previousStops) {
                    fail("Switching gradient type must preserve edited color stops");
                    return;
                }
                outputs.takeLive(document.slides(), 0);
            } else if (state->phase == 5) {
                const auto frame = ndi.lastFrame();
                if (frame.isNull() || frame.pixelColor(640, 360).red() < 245
                    || frame.pixelColor(5, 5).blue() < 240) {
                    fail("Radial gradient must expand from its center");
                    return;
                }
                frame.save(folder + "/radial-output.png");
                document.importBackground(
                    QUrl::fromLocalFile(QDir(folder).absoluteFilePath("green.svg")));
            } else if (state->phase == 6) {
                if (document.importing()) {
                    return;
                }
                auto fill = document.backgroundFill();
                if (fill.value("type") != "image"
                    || !fill.value("source").toString().contains("/media/")) {
                    fail("Background image must be imported into managed storage");
                    return;
                }
                fill["fit"] = "contain";
                document.setBackgroundFill(fill);
                outputs.takeLive(document.slides(), 0);
            } else if (state->phase == 7) {
                const auto frame = ndi.lastFrame();
                if (frame.isNull() || frame.pixelColor(640, 360) != QColor("#00FF00")
                    || frame.pixelColor(5, 360) != QColor("#3A86FF")) {
                    frame.save(folder + "/image-fit-failure.png");
                    fail(QString(
                        "Image Fit must preserve image and canvas color: center=%1, corner=%2")
                            .arg(frame.pixelColor(640, 360).name(QColor::HexArgb),
                                frame.pixelColor(5, 360).name(QColor::HexArgb)));
                    return;
                }
                frame.save(folder + "/image-fit-output.png");
                auto fill = document.backgroundFill();
                fill["fit"] = "cover";
                document.setBackgroundFill(fill);
                const auto saved = templates.saveTemplate(
                    "Image background", "song", document.slides().first().toMap());
                QString error;
                const QVariantMap song { { "title", "Image fill" },
                    { "sections",
                        QVariantList {
                            QVariantMap { { "key", "v1" }, { "primary", "Lyrics over image" } } } },
                    { "arrangement", QVariantList { "v1" } } };
                const auto pages
                    = TemplateComposer::songSlides(song, 3, templates.layout(saved), &error);
                if (pages.isEmpty() || !error.isEmpty()) {
                    fail("Image template composition");
                    return;
                }
                outputs.takeLive(pages, 0);
            } else {
                const auto frame = ndi.lastFrame();
                if (frame.isNull() || frame.pixelColor(5, 360) != QColor("#00FF00")) {
                    fail("Image Fill must cover the entire canvas");
                    return;
                }
                frame.save(folder + "/image-fill-output.png");
                timer->stop();
                ndi.stop();
                qInfo() << "Canvas fill checks passed: solid/transparent/linear/radial/image, "
                           "managed imports, template composition and GPU alpha.";
                qApp->exit(0);
                return;
            }
            ++state->phase;
        });
    timer->start();
    QTimer::singleShot(20000, &engine, [] { qApp->exit(323); });
}
