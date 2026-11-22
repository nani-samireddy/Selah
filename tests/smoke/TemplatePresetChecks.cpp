#include "TemplatePresetChecks.h"
#include "TemplatePresets.h"
#include "TemplateStore.h"
#include <memory>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QJSValue>
#include <QPainter>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>

namespace {
QList<QQuickItem *> children(QQuickItem *root) {
    QList<QQuickItem *> items { root };
    for (auto child : root->childItems()) {
        items.append(children(child));
    }
    return items;
}

struct Capture : std::enable_shared_from_this<Capture> {
    QTemporaryDir directory;
    TemplateStore store;
    QQmlApplicationEngine &engine;
    QString folder;
    QVariantList layouts;
    QQuickWindow window;
    QQuickItem *renderer = nullptr;
    QImage sheet;
    int index = 0;
    int overflows = 0;

    Capture(QQmlApplicationEngine &engine, const QString &folder)
        : store(nullptr, directory.path())
        , engine(engine)
        , folder(folder) { }

    void next() {
        if (index == layouts.size()) {
            if (!sheet.save(folder + "/contact-sheet.jpg", "JPG", 94)) {
                qApp->exit(275);
                return;
            }
            qInfo() << "Rendered 26 preset layouts; text overflows:" << overflows;
            qApp->exit(overflows ? 272 : 0);
            return;
        }
        const auto definition = layouts[index].toMap();
        const auto preview = store.preview(definition["id"].toString());
        const auto slides = preview["slides"].toList();
        if (!preview["error"].toString().isEmpty() || slides.isEmpty()) {
            qCritical() << definition["name"] << preview["error"];
            qApp->exit(271);
            return;
        }
        const auto slide = slides[0].toMap();
        renderer->setProperty("backgroundColor", slide["background"]);
        renderer->setProperty("ambientOpacity", slide["ambientOpacity"]);
        renderer->setProperty("elements", slide["elements"]);
        auto self = shared_from_this();
        QTimer::singleShot(350, qApp, [self, definition] {
            for (auto item : children(self->renderer)) {
                if (!item->objectName().startsWith("slide-text-")) {
                    continue;
                }
                if (item->property("contentHeight").toDouble() > item->height() + 1
                    || item->property("contentWidth").toDouble() > item->width() + 1) {
                    qCritical() << "Template text overflow:" << definition["name"]
                                << item->objectName() << item->property("contentWidth")
                                << item->width() << item->property("contentHeight")
                                << item->height();
                    ++self->overflows;
                }
            }
            const auto wrapped = self->engine.newQObject(self->renderer);
            for (const auto &entry :
                wrapped.property("probe").callWithInstance(wrapped).toVariant().toList()) {
                if (!entry.toMap()["problem"].toString().isEmpty()) {
                    qCritical() << "Template media failure:" << definition["name"] << entry;
                    qApp->exit(273);
                    return;
                }
            }
            const auto frame = self->window.grabWindow();
            if (frame.isNull()
                || !frame.save(self->folder + "/" + definition["id"].toString() + ".png")) {
                qApp->exit(274);
                return;
            }
            QPainter painter(&self->sheet);
            const int x = (self->index % 4) * 480, y = (self->index / 4) * 300;
            painter.drawImage(QRect(x, y, 480, 270), frame);
            painter.setPen(Qt::white);
            QFont font("Inter");
            font.setPixelSize(16);
            painter.setFont(font);
            painter.drawText(QRect(x + 10, y + 273, 460, 25), definition["name"].toString());
            painter.end();
            ++self->index;
            self->next();
        });
    }
};
}

void runTemplatePresetChecks(QQmlApplicationEngine &engine, const QString &folder) {
    QDir().mkpath(folder);
    auto capture = std::make_shared<Capture>(engine, folder);
    const auto error = TemplatePresets::install(capture->store, nullptr, capture->directory.path());
    if (!error.isEmpty()) {
        qCritical() << error;
        qApp->exit(270);
        return;
    }
    for (const auto &value : capture->store.layouts()) {
        if (value.toMap()["id"].toString().startsWith("preset-")) {
            capture->layouts.append(value);
        }
    }
    if (capture->layouts.size() != 26) {
        qApp->exit(270);
        return;
    }
    capture->sheet = QImage(1920, ((capture->layouts.size() + 3) / 4) * 300, QImage::Format_RGB32);
    capture->sheet.fill(QColor("#242424"));
    capture->window.setTitle("Selah template verification");
    capture->window.resize(960, 540);
    QQmlComponent component(&engine);
    component.loadFromModule("ChurchDeck", "SlideRenderer");
    capture->renderer = qobject_cast<QQuickItem *>(component.create());
    if (!capture->renderer) {
        qCritical() << component.errors();
        qApp->exit(270);
        return;
    }
    capture->renderer->setParent(&capture->window);
    capture->renderer->setParentItem(capture->window.contentItem());
    capture->renderer->setScale(0.5);
    capture->renderer->setTransformOrigin(QQuickItem::TopLeft);
    capture->renderer->setProperty("videoEnabled", false);
    capture->window.show();
    capture->next();
}
