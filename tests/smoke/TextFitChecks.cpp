#include "TextFitChecks.h"
#include "DocumentElements.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "TextFormatting.h"
#include "VisualLookup.h"
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

void runTextFitChecks(EditorDocument &document, OutputManager &outputs,
    QQmlApplicationEngine &engine, const QString &folder) {
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(500);
        return;
    }
    auto element = DocumentElements::text("Fit text",
        QString("God gives us hope and strength through every season. ").repeated(3), 530, 330, 850,
        260, 96, "Arial");
    element["uid"] = "fit-text";
    element["textSizing"] = "shrink";
    element["minFontSize"] = 32;
    TextFormatting format;
    element = format.apply(element, { { "fontWeight", 700 }, { "color", "#F0C578" } }, 0, 3);
    if (!document.replaceSlides(
            { QVariantMap { { "name", "Shrink to fit" }, { "background", "#20303C" },
                { "ambientOpacity", 0.0 }, { "elements", QVariantList { element } } } })) {
        qApp->exit(501);
        return;
    }
    document.setSelected(0);
    window->setProperty("activeWorkspace", "design");
    outputs.takeLive(document.slides(), 0, "fit-protected-snapshot");
    const auto snapshot = outputs.liveSlides();
    QDir().mkpath(folder);
    QTimer::singleShot(600, &engine, [&, window, element, snapshot, folder] {
        auto rendered = VisualLookup::find(window->contentItem(), "slide-text-fit-text");
        auto editor = VisualLookup::find(window->contentItem(), "richTextEditor");
        const auto display = rendered ? rendered->property("displayData").toMap() : QVariantMap();
        if (!rendered || !editor || display["fontSize"].toInt() >= 96
            || display["fontSize"].toInt() < 32 || display["text"] != element["text"]
            || rendered->property("contentHeight").toDouble() > 260) {
            qCritical() << "Rendered fit mismatch" << display
                        << (rendered ? rendered->property("contentHeight") : QVariant());
            qApp->exit(502);
            return;
        }
        window->grabWindow().save(folder + "/shrink-preview.png");
        QMetaObject::invokeMethod(editor, "begin", Q_ARG(QVariant, QVariant(0)));
        QTimer::singleShot(300, &engine, [&, window, editor, element, snapshot, folder] {
            const double scale = editor->property("fitScale").toDouble();
            if (!(scale > 0 && scale < 1) || qAbs(editor->width() * scale - 850) > 0.5) {
                qApp->exit(503);
                return;
            }
            window->grabWindow().save(folder + "/shrink-editing.png");
            QMetaObject::invokeMethod(window, "update", Q_ARG(QVariant, QVariant("minFontSize")),
                Q_ARG(QVariant, QVariant(40)));
            if (document.selectedElement()["minFontSize"].toInt() != 40) {
                qApp->exit(515);
                return;
            }
            QMetaObject::invokeMethod(editor, "finish");
            if (document.selectedElement()["fontSize"].toInt() != 96
                || document.selectedElement()["text"] != element["text"]
                || document.selectedElement()["width"].toInt() != 850
                || outputs.liveSlides() != snapshot) {
                qApp->exit(504);
                return;
            }
            auto inspector = VisualLookup::find(window->contentItem(), "textInspector");
            inspector->setProperty("advanced", true);
            for (const auto &name :
                { "text_textCase_upper", "text_listStyle_bullet", "text_baseline_super" }) {
                auto button = VisualLookup::find(window->contentItem(), name);
                if (!button) {
                    qApp->exit(505);
                    return;
                }
                QMetaObject::invokeMethod(button, "clicked");
            }
            auto undo = VisualLookup::find(window->contentItem(), "editorUndoButton");
            auto redo = VisualLookup::find(window->contentItem(), "editorRedoButton");
            bool inDock = false;
            for (auto parent = undo ? undo->parentItem() : nullptr; parent;
                parent = parent->parentItem()) {
                inDock |= parent->objectName() == "editorFloatingDock";
            }
            if (!undo || !redo || !inDock || document.selectedElement()["textCase"] != "upper"
                || document.selectedElement()["listStyle"] != "bullet"
                || document.selectedElement()["baseline"] != "super") {
                qApp->exit(506);
                return;
            }
            QMetaObject::invokeMethod(undo, "clicked");
            document.setSelected(0);
            if (document.selectedElement()["baseline"] != "normal") {
                qApp->exit(507);
                return;
            }
            QMetaObject::invokeMethod(redo, "clicked");
            document.setSelected(0);
            if (document.selectedElement()["baseline"] != "super") {
                qApp->exit(508);
                return;
            }
            auto units = VisualLookup::find(window->contentItem(), "inspectorUnitButton");
            if (!units) {
                qApp->exit(509);
                return;
            }
            QMetaObject::invokeMethod(units, "clicked");
            QTimer::singleShot(200, &engine, [&, window, snapshot, folder] {
                auto menu = window->findChild<QObject *>("lineHeightUnitMenu");
                auto pixels = window->findChild<QObject *>("lineHeightPixelsOption");
                auto percent = window->findChild<QObject *>("lineHeightPercentOption");
                if (!menu || !menu->property("opened").toBool() || !pixels || !percent) {
                    qApp->exit(510);
                    return;
                }
                QMetaObject::invokeMethod(pixels, "triggered");
                if (document.selectedElement()["lineHeightUnit"] != "px"
                    || qAbs(document.selectedElement()["lineHeight"].toDouble() - 96) > 0.01) {
                    qApp->exit(511);
                    return;
                }
                QMetaObject::invokeMethod(percent, "triggered");
                QMetaObject::invokeMethod(menu, "close");
                if (document.selectedElement()["lineHeightUnit"] != "percent"
                    || qAbs(document.selectedElement()["lineHeight"].toDouble() - 1) > 0.01
                    || outputs.liveSlides() != snapshot) {
                    qApp->exit(513);
                    return;
                }
                auto file = VisualLookup::find(window->contentItem(), "editorFileActionsButton");
                QMetaObject::invokeMethod(file, "clicked");
                QTimer::singleShot(200, &engine, [&, window, folder] {
                    auto menu = window->findChild<QObject *>("editorFileMenu");
                    if (!menu || !menu->property("opened").toBool()) {
                        qApp->exit(514);
                        return;
                    }
                    window->grabWindow().save(folder + "/editor-actions.png");
                    QMetaObject::invokeMethod(menu, "close");
                    auto scroll
                        = VisualLookup::find(window->contentItem(), "inspectorPropertyScroll");
                    auto viewport = qobject_cast<QQuickItem *>(
                        scroll->property("contentItem").value<QObject *>());
                    auto target = VisualLookup::find(window->contentItem(), "text_textCase_upper");
                    if (viewport && target) {
                        viewport->setProperty("contentY",
                            qMax(0.0,
                                viewport->property("contentY").toDouble()
                                    + target->mapToItem(viewport, QPointF()).y() - 20));
                    }
                    QTimer::singleShot(200, &engine, [window, folder] {
                        window->grabWindow().save(folder + "/text-option-groups.png");
                        qInfo() << "Text fit and controls checks passed: fit, rich editing, "
                                   "protected output, grouped case/list/baseline, inline unit "
                                   "conversion, dock undo/redo and file actions.";
                        qApp->exit(0);
                    });
                });
            });
        });
    });
}
