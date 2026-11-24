#include "TemplateChecks.h"
#include "EditorDocument.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "TemplateDefinition.h"
#include "TemplateStore.h"
#include "TemplateTextLayout.h"
#include "VisualLookup.h"
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QJSValue>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

namespace {
QList<QQuickItem *> visualItems(QQuickItem *root) {
    QList<QQuickItem *> result { root };
    for (auto child : root->childItems()) {
        result.append(visualItems(child));
    }
    return result;
}
}

void runTemplateChecks(TemplateStore &templates, ProgramStore &programs, EditorDocument &editor,
    OutputManager &outputs, QQmlApplicationEngine &engine, const QString &folder, bool compact) {
    QDir().mkpath(folder);
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) {
        qApp->exit(150);
        return;
    }
    if (compact) {
        window->setWidth(1120);
        window->setHeight(760);
    }
    auto definition = TemplateDefinition::starter("bilingual");
    auto slide = definition["slide"].toMap();
    auto elements = slide["elements"].toList();
    auto primary = elements[1].toMap();
    primary["x"] = 180;
    primary["width"] = 1200;
    primary["height"] = 200;
    primary["fontSize"] = 110;
    primary["alignment"] = "left";
    elements[1] = primary;
    auto translated = elements[2].toMap();
    translated["alignment"] = "left";
    translated["x"] = 180;
    elements[2] = translated;
    slide["elements"] = elements;
    slide["background"] = "#173C40";
    slide["ambientOpacity"] = 0.0;
    const auto id = templates.saveTemplate("Worship · bilingual narrow", "bilingual", slide);
    if (id.isEmpty()) {
        qCritical() << templates.status();
        qApp->exit(151);
        return;
    }
    definition = templates.layout(id);
    const QVariantMap song { { "title", "Together in hope" },
        { "credits", "Original verification lyrics" },
        { "sections",
            QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                { "primary",
                    "We gather in Your light\nWe lift our hearts in praise\nWe share a living "
                    "hope" },
                { "secondary",
                    "నీ వెలుగులో కూడుదము\nస్తుతితో హృదయాలను పైకెత్తుదము\nజీవముగల ఆశను పంచుదము" } } } },
        { "arrangement", QVariantList { "v1" } } };
    if (!programs.createProgram("Template service", "") || !programs.addSong(song, 3)
        || !programs.present(0)) {
        qApp->exit(152);
        return;
    }
    const auto before = outputs.liveElements();
    const auto editingModeBefore = window->property("templateMode");
    auto panel = engine.newQObject(window->findChild<QObject *>("templatePanel"));
    panel.setProperty("programTarget", engine.newQObject(&programs));
    panel.setProperty("selectedId", id);
    panel.property("open").callWithInstance(panel);
    panel.setProperty("searchQuery", "Worship · bilingual narrow");
    QTimer::singleShot(
        650, qApp, [&, window, folder, panel, id, before, editingModeBefore]() mutable {
            auto card = VisualLookup::find(window->contentItem(), "templateCard_" + id);
            if (!card) {
                qApp->exit(159);
                return;
            }
            // Padding is part of the card's hit area too. Program pickers still preview.
            const auto point = card->mapToScene(QPointF(4, 4));
            const QPointF global = window->mapToGlobal(point.toPoint());
            QMouseEvent press(QEvent::MouseButtonPress, point, global, Qt::LeftButton,
                Qt::LeftButton, Qt::NoModifier);
            QMouseEvent release(QEvent::MouseButtonRelease, point, global, Qt::LeftButton,
                Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &press);
            QCoreApplication::sendEvent(window, &release);
            if (!panel.property("inspecting").toBool()
                || window->property("templateMode") != editingModeBefore) {
                qCritical() << "Program template picker must preview after clicking card padding";
                qApp->exit(159);
                return;
            }
            const auto result = panel.property("result").toVariant().toMap();
            if (!result["error"].toString().isEmpty() || result["slides"].toList().size() < 2) {
                qCritical() << result["error"];
                qApp->exit(153);
                return;
            }
            if (!window->findChild<QObject *>("applyLayoutButton")->property("enabled").toBool()) {
                qCritical() << "Apply layout is disabled for a valid preview";
                qApp->exit(158);
                return;
            }
            window->grabWindow().save(folder + "/template-content-preview.png");
            if (!programs.applyLayout(templates.layout(id)) || outputs.liveElements() != before) {
                qApp->exit(154);
                return;
            }
            panel.property("close").callWithInstance(panel);
            programs.present(0);
            outputs.preview("audience");
            QTimer::singleShot(250, qApp, [&, window, folder, id] {
                for (auto output : QGuiApplication::topLevelWindows()) {
                    if (output->title().contains("Audience output")) {
                        qobject_cast<QQuickWindow *>(output)->grabWindow().save(
                            folder + "/template-audience.png");
                    }
                }
                int measuredTextBoxes = 0;
                for (auto output : QGuiApplication::topLevelWindows()) {
                    if (output->title().contains("Audience output")) {
                        for (auto item :
                            visualItems(qobject_cast<QQuickWindow *>(output)->contentItem())) {
                            if (item->objectName().startsWith("slide-text-")) {
                                ++measuredTextBoxes;
                                if (item->property("contentHeight").toDouble()
                                        > item->height() + 0.5
                                    || item->property("contentWidth").toDouble()
                                        > item->width() + 0.5) {
                                    qCritical()
                                        << "Rendered template text exceeds its box"
                                        << item->objectName() << item->property("contentHeight")
                                        << item->height();
                                    qApp->exit(157);
                                    return;
                                }
                            }
                        }
                    }
                }
                if (measuredTextBoxes < 2) {
                    qCritical() << "No rendered template text metrics were found";
                    qApp->exit(157);
                    return;
                }
                outputs.stop("audience");
                auto root = engine.newQObject(window);
                root.property("editTemplate")
                    .callWithInstance(root, { engine.toScriptValue(templates.layout(id)) });
                editor.setSelected(1);
                QTimer::singleShot(300, qApp, [&, window, folder, id] {
                    QQuickItem *renderer = nullptr;
                    for (auto item : visualItems(window->contentItem())) {
                        if (item->objectName() == "editorSlideRenderer") {
                            renderer = item;
                        }
                    }
                    if (!renderer
                        || renderer->property("backgroundColor").value<QColor>()
                            != QColor("#173C40")) {
                        qCritical() << "Editor background did not follow the template";
                        qApp->exit(159);
                        return;
                    }
                    window->grabWindow().save(folder + "/template-editor.png");
                    const auto live = outputs.liveElements();
                    editor.updateElement(1, { { "width", 700 }, { "fontSize", 100 } });
                    if (templates
                            .saveTemplate("Worship · bilingual narrow", "bilingual",
                                editor.slides()[0].toMap(), id)
                            .isEmpty()
                        || programs.selectedItem()["layoutTemplate"].toMap()["revision"] != 1
                        || outputs.liveElements() != live) {
                        qApp->exit(155);
                        return;
                    }
                    if (!programs.applyLayout(templates.layout(id))
                        || outputs.liveElements() != live) {
                        qApp->exit(156);
                        return;
                    }
                    programs.present(0);
                    outputs.preview("audience");
                    QTimer::singleShot(250, qApp, [&, folder] {
                        for (auto output : QGuiApplication::topLevelWindows()) {
                            if (output->title().contains("Audience output")) {
                                qobject_cast<QQuickWindow *>(output)->grabWindow().save(
                                    folder + "/template-reflow-output.png");
                            }
                        }
                        outputs.stop("audience");
                        programs.save();
                        qInfo()
                            << "Template smoke passed: real-content preview, custom editor "
                               "bindings, "
                               "box-driven reflow, versioned snapshots, and protected live output.";
                        qApp->exit(0);
                    });
                });
            });
        });
}
