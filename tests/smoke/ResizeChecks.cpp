#include "ResizeChecks.h"
#include "DocumentElements.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "VisualLookup.h"
#include <cmath>
#include <memory>
#include <numbers>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QWheelEvent>

namespace {
void pointer(QQuickWindow *window, QEvent::Type type, const QPointF &point,
    Qt::MouseButton pressedButton = Qt::LeftButton) {
    const auto button = type == QEvent::MouseMove ? Qt::NoButton : pressedButton;
    const auto buttons = type == QEvent::MouseButtonRelease ? Qt::NoButton : pressedButton;
    QMouseEvent event(
        type, point, window->mapToGlobal(point.toPoint()), button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &event);
}

QPointF cornerPoint(const QVariantMap &rect, int corner) {
    const double angle = rect["rotation"].toDouble() * std::numbers::pi / 180;
    const double x = (corner % 2 ? 1 : -1) * rect["width"].toDouble() / 2;
    const double y = (corner > 1 ? 1 : -1) * rect["height"].toDouble() / 2;
    return { rect["x"].toDouble() + rect["width"].toDouble() / 2 + std::cos(angle) * x
            - std::sin(angle) * y,
        rect["y"].toDouble() + rect["height"].toDouble() / 2 + std::sin(angle) * x
            + std::cos(angle) * y };
}
}

void runResizeChecks(EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(310);
        return;
    }
    window->resize(1440, 950);
    window->raise();
    window->requestActivate();
    window->setProperty("templateMode", false);
    window->setProperty("snap", false);
    QDir().mkpath(folder);
    QFile image(QDir(folder).absoluteFilePath("resize-image.svg"));
    if (!image.open(QIODevice::WriteOnly)) {
        qApp->exit(311);
        return;
    }
    image.write("<svg xmlns='http://www.w3.org/2000/svg' width='500' height='220'><rect "
                "width='500' height='220' fill='#407B64'/></svg>");
    image.close();
    const auto source = QUrl::fromLocalFile(image.fileName()).toString();
    const auto baseline = document.slides();
    outputs.takeLive(baseline, 0, "resize-protected-live");
    const auto snapshot = outputs.snapshotId();

    struct State {
        int test = 0;
        int phase = -4;
        QVariantMap before;
        QVariantList beforeCollapse;
        int collapseWaits = 0;
        int genieWaits = 0;
        int compactPhase = 0;
        QPointF start, finish;
    };

    auto state = std::make_shared<State>();
    auto timer = new QTimer(&engine);
    timer->setInterval(200);
    QObject::connect(
        timer, &QTimer::timeout, &engine, [&, state, timer, window, source, snapshot, folder] {
            auto fail = [&](const QString &error) {
                qCritical() << "Resize check failed:" << state->test << error;
                timer->stop();
                qApp->exit(312);
            };
            const int handle = state->test % 4;
            if (state->phase < 0) {
                auto viewport = VisualLookup::find(window->contentItem(), "editorCanvasViewport");
                auto layers = VisualLookup::find(window->contentItem(), "editorLayersPanel");
                auto inspector = VisualLookup::find(window->contentItem(), "editorInspectorPanel");
                auto dock = VisualLookup::find(window->contentItem(), "editorFloatingDock");
                auto artboard = VisualLookup::find(window->contentItem(), "editorArtboard");
                if (!viewport || !layers || !inspector || !dock || !artboard
                    || viewport->width() != viewport->parentItem()->width()
                    || viewport->height() != viewport->parentItem()->height()) {
                    fail("Canvas viewport must fill the workspace behind floating panels");
                    return;
                }
                auto genie = VisualLookup::find(window->contentItem(), "slidesPanelMotion");
                auto tray = VisualLookup::find(window->contentItem(), "editorFloatingControls");
                auto toggle = VisualLookup::find(window->contentItem(), "toggleEditorSlidesPanel");
                if (!genie || !tray || !toggle) {
                    fail("Slides animation controls missing");
                    return;
                }
                if (state->phase == -4) {
                    const auto beforeHistory = document.slides();
                    document.addText();
                    const auto afterHistory = document.slides();
                    QMetaObject::invokeMethod(window, "selectCanvasSettings");
                    auto historyKey = [&](QKeySequence::StandardKey standard) {
                        const auto combination = QKeySequence(standard)[0];
                        QKeyEvent press(
                            QEvent::KeyPress, combination.key(), combination.keyboardModifiers());
                        QKeyEvent release(
                            QEvent::KeyRelease, combination.key(), combination.keyboardModifiers());
                        QCoreApplication::sendEvent(window, &press);
                        QCoreApplication::sendEvent(window, &release);
                    };
                    historyKey(QKeySequence::Undo);
                    if (document.slides() != beforeHistory) {
                        fail("Undo shortcut must restore canvas content");
                        return;
                    }
                    historyKey(QKeySequence::Redo);
                    if (document.slides() != afterHistory) {
                        fail("Redo shortcut must restore the undone canvas edit");
                        return;
                    }
                    auto undo = VisualLookup::find(window->contentItem(), "editorUndoButton");
                    auto redo = VisualLookup::find(window->contentItem(), "editorRedoButton");
                    if (!undo || !redo || !undo->isEnabled()) {
                        fail("Persistent editor history buttons must be available");
                        return;
                    }
                    QMetaObject::invokeMethod(undo, "clicked");
                    if (document.slides() != beforeHistory || !redo->isEnabled()) {
                        fail("Toolbar Undo must share the keyboard history");
                        return;
                    }
                    document.addText();
                    auto richEditor = window->findChild<QObject *>("richTextEditor");
                    QMetaObject::invokeMethod(
                        richEditor, "begin", Q_ARG(QVariant, QVariant(document.selected())));
                    const auto beforeFormat = document.slides();
                    QMetaObject::invokeMethod(richEditor, "applyFormat",
                        Q_ARG(QVariant, QVariant(QVariantMap { { "fontWeight", 700 } })));
                    const auto formatted = document.slides();
                    historyKey(QKeySequence::Undo);
                    if (formatted == beforeFormat || document.slides() != beforeFormat) {
                        qWarning() << "Text history" << (formatted == beforeFormat)
                                   << window->property("editingId")
                                   << document.selectedElement().value("fontWeight");
                        QFile receipt(folder + "/text-history.json");
                        if (receipt.open(QIODevice::WriteOnly)) {
                            receipt.write(
                                QJsonDocument::fromVariant(QVariantMap { { "before", beforeFormat },
                                                               { "formatted", formatted },
                                                               { "afterUndo", document.slides() } })
                                    .toJson());
                        }
                        fail("Undo must restore rich text formatting from the document history");
                        return;
                    }
                    historyKey(QKeySequence::Redo);
                    if (document.slides() != formatted) {
                        fail("Redo must restore rich text formatting");
                        return;
                    }
                    document.replaceSlides(beforeHistory);
                    QMetaObject::invokeMethod(window, "selectCanvasSettings");
                    const auto beforeGestures = document.slides();
                    const QPointF local(viewport->width() * 0.55, viewport->height() * 0.4);
                    const auto scene = viewport->mapToScene(local);
                    auto wheel = [&](QPoint pixels, QPoint angles, Qt::KeyboardModifiers modifiers,
                                     const QPointF &at) {
                        QWheelEvent event(at, window->mapToGlobal(at.toPoint()), pixels, angles,
                            Qt::NoButton, modifiers, Qt::ScrollUpdate, false);
                        QCoreApplication::sendEvent(window, &event);
                    };
                    wheel({ 23, -17 }, {}, Qt::NoModifier, scene);
                    if (window->property("panX").toDouble() != 23
                        || window->property("panY").toDouble() != -17
                        || window->property("zoom").toDouble() != 1) {
                        fail("Two-finger scrolling must pan instead of zooming");
                        return;
                    }
                    const auto beforeScale = artboard->scale();
                    const auto anchor = artboard->mapFromScene(scene);
                    wheel({}, { 0, 120 }, Qt::ControlModifier, scene);
                    if (artboard->scale() <= beforeScale
                        || (artboard->mapFromScene(scene) - anchor).manhattanLength() > 0.01) {
                        fail("Wheel zoom must preserve the canvas point under the cursor");
                        return;
                    }
                    const auto zoom = window->property("zoom").toDouble();
                    wheel({}, {}, Qt::NoModifier, scene);
                    wheel({}, { 0, 120 }, Qt::NoModifier, layers->mapToScene(QPointF(10, 10)));
                    if (window->property("zoom").toDouble() != zoom) {
                        fail("Zero-delta events and floating panels must not zoom the canvas");
                        return;
                    }
                    static const QPointingDevice touchpad("Smoke trackpad", 999,
                        QInputDevice::DeviceType::TouchPad, QPointingDevice::PointerType::Finger,
                        QInputDevice::Capability::Position, 2, 0);
                    auto native = [&](Qt::NativeGestureType type, qreal value) {
                        QNativeGestureEvent event(type, &touchpad, 2, scene, scene,
                            window->mapToGlobal(scene.toPoint()), value, {});
                        QCoreApplication::sendEvent(window, &event);
                    };
                    const auto pinchAnchor = artboard->mapFromScene(scene);
                    native(Qt::BeginNativeGesture, 0);
                    native(Qt::ZoomNativeGesture, 0.2);
                    native(Qt::EndNativeGesture, 0);
                    if (window->property("zoom").toDouble() <= zoom
                        || (artboard->mapFromScene(scene) - pinchAnchor).manhattanLength() > 0.01
                        || document.slides() != beforeGestures
                        || outputs.snapshotId() != snapshot) {
                        qWarning() << "Pinch" << zoom << window->property("zoom") << pinchAnchor
                                   << artboard->mapFromScene(scene)
                                   << (document.slides() == beforeGestures)
                                   << (outputs.snapshotId() == snapshot);
                        fail("Native pinch must zoom about its anchor without editing content");
                        return;
                    }
                    const QPointF panBeforeMiddle(
                        window->property("panX").toDouble(), window->property("panY").toDouble());
                    pointer(window, QEvent::MouseButtonPress, scene, Qt::MiddleButton);
                    pointer(window, QEvent::MouseMove, scene + QPointF(30, 20), Qt::MiddleButton);
                    pointer(window, QEvent::MouseButtonRelease, scene + QPointF(30, 20),
                        Qt::MiddleButton);
                    const QPointF panAfterMiddle(
                        window->property("panX").toDouble(), window->property("panY").toDouble());
                    if ((panAfterMiddle - panBeforeMiddle - QPointF(30, 20)).manhattanLength()
                            > 0.01
                        || document.slides() != beforeGestures) {
                        qWarning() << "Middle pan" << panBeforeMiddle << panAfterMiddle
                                   << (document.slides() == beforeGestures);
                        fail("Middle-button drag must pan over elements without moving them");
                        return;
                    }
                    window->setProperty("zoom", 1);
                    window->setProperty("panX", 0);
                    window->setProperty("panY", 0);
                    if (tray->mapToScene(QPointF()).y()
                        < dock->mapToScene(QPointF(0, dock->height())).y()) {
                        fail("Slides panel must sit below the floating toolbar");
                        return;
                    }
                    QMetaObject::invokeMethod(toggle, "clicked");
                    auto clock = genie->findChild<QObject *>("slidesPanelAnimationClock");
                    if (!clock || !genie->property("animating").toBool()) {
                        qWarning()
                            << "Genie start" << clock << genie->property("animating")
                            << genie->property("motionEnabled") << genie->property("supported")
                            << genie->property("expanded") << genie->property("progress")
                            << toggle->property("checked") << window->property("templateMode");
                        fail("Panel toggle must start its animation");
                        return;
                    }
                    clock->setProperty("paused", true);
                    genie->setProperty("progress", 0.5);
                    ++state->phase;
                    return;
                }
                if (state->phase == -3) {
                    const auto progress = genie->property("progress").toDouble();
                    if (!genie->property("animating").toBool() || progress <= 0 || progress >= 1) {
                        fail("Hiding slides must animate into the toolbar");
                        return;
                    }
                    window->grabWindow().save(folder + "/slides-genie-midpoint.png");
                    genie->findChild<QObject *>("slidesPanelAnimationClock")
                        ->setProperty("paused", false);
                    QMetaObject::invokeMethod(toggle, "clicked");
                    ++state->phase;
                    return;
                }
                if (state->phase == -2 && genie->property("animating").toBool()
                    && ++state->genieWaits < 15) {
                    return;
                }
                if (state->phase == -2) {
                    if (state->compactPhase == 0) {
                        if (!tray->isVisible() || !toggle->property("checked").toBool()) {
                            fail("The full slides panel must start expanded");
                            return;
                        }
                        state->beforeCollapse = document.slides();
                        QMetaObject::invokeMethod(toggle, "clicked");
                        state->genieWaits = 0;
                        state->compactPhase = 1;
                        return;
                    }
                    if (state->compactPhase == 1) {
                        auto compact
                            = VisualLookup::find(window->contentItem(), "compactEditorSlides");
                        if (!compact || !compact->isVisible() || tray->isVisible()
                            || compact->height() != dock->height()
                            || std::abs(compact->mapToScene(QPointF()).y()
                                   - dock->mapToScene(QPointF()).y())
                                > 1
                            || document.slides() != state->beforeCollapse) {
                            fail(
                                "Minimized slides must remain as a toolbar-height thumbnail strip");
                            return;
                        }
                        window->grabWindow().save(folder + "/compact-slides.png");
                        QMetaObject::invokeMethod(toggle, "clicked");
                        state->genieWaits = 0;
                        state->compactPhase = 2;
                        return;
                    }
                    if (!tray->isVisible() || !toggle->property("checked").toBool()) {
                        fail("Expanding compact slides must restore slides and notes");
                        return;
                    }
                }
                if (state->phase == -1) {
                    if (layers->property("collapsed").toBool()
                        && inspector->property("collapsed").toBool()
                        && (layers->height() > 51 || inspector->height() > 51)
                        && ++state->collapseWaits < 15) {
                        return;
                    }
                    auto layerBody = VisualLookup::find(window->contentItem(), "layersPanelBody");
                    auto propertyBody
                        = VisualLookup::find(window->contentItem(), "propertiesPanelBody");
                    if (!layers->property("collapsed").toBool()
                        || !inspector->property("collapsed").toBool() || layers->height() > 51
                        || inspector->height() > 51 || !layerBody || layerBody->isVisible()
                        || !propertyBody || propertyBody->isVisible()
                        || document.slides() != state->beforeCollapse) {
                        window->grabWindow().save(folder + "/collapse-failure.png");
                        qWarning() << "Collapsed state" << layers->property("collapsed")
                                   << inspector->property("collapsed") << layers->height()
                                   << inspector->height() << layerBody << propertyBody
                                   << (layerBody && layerBody->isVisible())
                                   << (propertyBody && propertyBody->isVisible())
                                   << (document.slides() == state->beforeCollapse);
                        fail("Collapsed panels must leave only headers and preserve the document");
                        return;
                    }
                    window->grabWindow().save(folder + "/collapsed-panels.png");
                }
                window->setProperty("handTool", true);
                const auto panBefore = QPointF(
                    window->property("panX").toDouble(), window->property("panY").toDouble());
                const auto layersBefore = layers->mapToScene(QPointF());
                const auto inspectorBefore = inspector->mapToScene(QPointF());
                const auto dockBefore = dock->mapToScene(QPointF());
                const auto artboardBefore = artboard->mapToScene(QPointF());
                const auto start = state->phase == -2
                    ? viewport->mapToScene(QPointF(viewport->width() / 2, viewport->height() / 2))
                    : layers->mapToScene(QPointF(4, layers->height() - 4));
                const QPointF delta(120, 70);
                pointer(window, QEvent::MouseButtonPress, start);
                pointer(window, QEvent::MouseMove, start + delta);
                pointer(window, QEvent::MouseButtonRelease, start + delta);
                const auto panAfter = QPointF(
                    window->property("panX").toDouble(), window->property("panY").toDouble());
                if ((state->phase == -2 && (panAfter - panBefore - delta).manhattanLength() > 1)
                    || (state->phase == -1 && panAfter != panBefore)
                    || layers->mapToScene(QPointF()) != layersBefore
                    || inspector->mapToScene(QPointF()) != inspectorBefore
                    || dock->mapToScene(QPointF()) != dockBefore
                    || (state->phase == -2
                        && (artboard->mapToScene(QPointF()) - artboardBefore - delta)
                                .manhattanLength()
                            > 1)) {
                    window->grabWindow().save(folder + "/floating-failure.png");
                    qWarning() << "Floating phase" << state->phase << "pan" << panBefore << panAfter
                               << "artboard" << artboardBefore << artboard->mapToScene(QPointF())
                               << "layers" << layersBefore << layers->mapToScene(QPointF())
                               << "inspector" << inspectorBefore << inspector->mapToScene(QPointF())
                               << "dock" << dockBefore << dock->mapToScene(QPointF());
                    fail("Only the canvas should pan; floating panel surfaces must block pointer "
                         "events");
                    return;
                }
                if (state->phase == -2) {
                    window->setProperty("zoom", 1.6);
                    window->grabWindow().save(folder + "/floating-canvas-panned.png");
                    auto layerToggle
                        = VisualLookup::find(window->contentItem(), "toggleLayersPanel");
                    auto propertyToggle
                        = VisualLookup::find(window->contentItem(), "togglePropertiesPanel");
                    if (!layerToggle || !propertyToggle) {
                        fail("Panel collapse controls missing");
                        return;
                    }
                    state->beforeCollapse = document.slides();
                    QMetaObject::invokeMethod(layerToggle, "clicked");
                    QMetaObject::invokeMethod(propertyToggle, "clicked");
                } else {
                    QMetaObject::invokeMethod(
                        VisualLookup::find(window->contentItem(), "toggleLayersPanel"), "clicked");
                    QMetaObject::invokeMethod(window, "selectCanvasSettings");
                    if (layers->property("collapsed").toBool()
                        || inspector->property("collapsed").toBool()) {
                        fail("Panel controls and canvas settings must restore the expanded panels");
                        return;
                    }
                    window->setProperty("zoom", 1.0);
                    window->setProperty("panX", 0);
                    window->setProperty("panY", 0);
                    window->setProperty("handTool", false);
                }
                ++state->phase;
                return;
            }
            if (state->phase == 0) {
                if (state->test >= 15) {
                    timer->stop();
                    qInfo() << "Resize checks passed: four text/image corners, zoom, rotation, "
                               "rich-text editing, overlapping layers, snapping, bounds, "
                               "cancellation, locking and one-step undo.";
                    qApp->exit(0);
                    return;
                }
                auto element = DocumentElements::text(
                    "Resize target", "Drag any corner", 500, 300, 500, 220, 64, "Inter");
                if (state->test >= 4 && state->test < 8) {
                    element["kind"] = "image";
                    element["source"] = source;
                    element["fit"] = "contain";
                }
                if (state->test == 8) {
                    element["rotation"] = 30;
                }
                if (state->test == 11) {
                    element["locked"] = true;
                }
                auto cover = DocumentElements::text(
                    "Overlapping layer", "", 300, 200, 1200, 600, 36, "Inter");
                // A later/larger layer previously intercepted every earlier layer's handles.
                document.replaceSlides({ QVariantMap { { "name", "Resize checks" },
                    { "background", "#243644" }, { "ambientOpacity", 0 },
                    { "elements", QVariantList { element, cover } } } });
                document.setSelected(0);
                window->setProperty("snap", state->test == 9);
                window->setProperty("zoom", state->test >= 4 && state->test < 8 ? 1.5 : 1.0);
                if (state->test == 10) {
                    auto editor = VisualLookup::find(window->contentItem(), "richTextEditor");
                    QMetaObject::invokeMethod(editor, "begin", Q_ARG(QVariant, QVariant(0)));
                    QMetaObject::invokeMethod(editor, "commit");
                }
                state->before = document.selectedElement();
                ++state->phase;
            } else if (state->phase == 1) {
                auto grip = VisualLookup::find(
                    window->contentItem(), "editorResizeHandle_" + QString::number(handle));
                auto artboard = VisualLookup::find(window->contentItem(), "editorArtboard");
                if (!grip || !artboard) {
                    fail("Missing visible corner or artboard");
                    return;
                }
                if (state->test == 11) {
                    if (grip->isVisible()) {
                        fail("Locked layers should not expose active resize grips");
                        return;
                    }
                    ++state->test;
                    state->phase = 0;
                    return;
                }
                QPointF hit(grip->width() / 2, grip->height() / 2);
                if (state->test == 10) {
                    // Press inside the text box, within the enlarged handle hit target.
                    const double inset = grip->width() / 3;
                    hit += QPointF(handle % 2 ? -inset : inset, handle > 1 ? -inset : inset);
                }
                state->start = grip->mapToScene(hit);
                const double angle = state->before["rotation"].toDouble() * std::numbers::pi / 180;
                const double magnitude = state->test == 13 ? 4000 : state->test == 14 ? -4000 : 96;
                const double vertical = state->test >= 13 ? magnitude : 64;
                const double dx = (handle % 2 ? 1 : -1) * magnitude;
                const double dy = (handle > 1 ? 1 : -1) * vertical;
                const auto startOnArtboard = artboard->mapFromScene(state->start);
                state->finish = artboard->mapToScene(startOnArtboard
                    + QPointF(std::cos(angle) * dx - std::sin(angle) * dy,
                        std::sin(angle) * dx + std::cos(angle) * dy));
                pointer(window, QEvent::MouseButtonPress, state->start);
                pointer(window, QEvent::MouseMove, (state->start + state->finish) / 2);
                pointer(window, QEvent::MouseMove, state->finish);
                // No document mutations while dragging. Only the renderer/handles preview changes.
                if (document.selectedElement() != state->before) {
                    fail("Resize must stay transient until release");
                    return;
                }
                if (state->test == 12) {
                    // Switching tools cancels the active handle's mouse grab.
                    window->setProperty("handTool", true);
                }
                pointer(window, QEvent::MouseButtonRelease, state->finish);
                ++state->phase;
            } else {
                const auto after = document.selectedElement();
                if (state->test == 12) {
                    window->setProperty("handTool", false);
                    if (after != state->before) {
                        fail("Cancelled resize changed the document");
                        return;
                    }
                    ++state->test;
                    state->phase = 0;
                    return;
                }
                if (state->test == 13) {
                    if (after["x"].toDouble() < -0.001 || after["y"].toDouble() < -0.001
                        || after["x"].toDouble() + after["width"].toDouble() > 1920.001
                        || after["y"].toDouble() + after["height"].toDouble() > 1080.001) {
                        fail("Resizing beyond the canvas must be bounded");
                        return;
                    }
                } else if (state->test == 14) {
                    if (after["width"].toDouble() != 40 || after["height"].toDouble() != 30) {
                        fail("Shrinking must respect minimum dimensions");
                        return;
                    }
                } else if (qAbs(
                               after["width"].toDouble() - (state->before["width"].toDouble() + 96))
                        > 4
                    || qAbs(after["height"].toDouble() - (state->before["height"].toDouble() + 64))
                        > 4) {
                    fail(QString("Corner did not resize: %1×%2 -> %3×%4")
                            .arg(state->before["width"].toDouble())
                            .arg(state->before["height"].toDouble())
                            .arg(after["width"].toDouble())
                            .arg(after["height"].toDouble()));
                    return;
                }
                if ((cornerPoint(after, 3 - handle) - cornerPoint(state->before, 3 - handle))
                        .manhattanLength()
                    > 0.1) {
                    fail("Opposite corner moved");
                    return;
                }
                if (outputs.snapshotId() != snapshot) {
                    fail("Resizing changed live output");
                    return;
                }
                if (state->test == 0 || state->test == 8 || state->test == 10) {
                    window->grabWindow().save(
                        folder + "/resize-" + QString::number(state->test) + ".png");
                }
                document.undo();
                document.setSelected(0);
                if (document.selectedElement() != state->before) {
                    fail("A resize must undo in one step");
                    return;
                }
                ++state->test;
                state->phase = 0;
            }
        });
    timer->start();
    QTimer::singleShot(30000, &engine, [] { qApp->exit(313); });
}
