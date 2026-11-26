#include "RemotePreview.h"
#include "qrcodegen.hpp"
#include <QAbstractTextDocumentLayout>
#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QTextDocument>
#include <QUrl>

namespace {
QImage readImage(const QUrl &url) {
    if (!url.isLocalFile()) {
        return {};
    }
    QImageReader reader(url.toLocalFile());
    reader.setAutoTransform(true);
    const auto size = reader.size();
    if (size.isValid()) {
        reader.setScaledSize(size.scaled(960, 540, Qt::KeepAspectRatio));
    }
    return reader.read();
}

void drawImage(QPainter &painter, const QImage &image, const QRectF &box, const QString &fit) {
    if (image.isNull()) {
        return;
    }
    QRectF source(QPointF(), image.size()), target = box;
    if (fit == "contain") {
        const auto size = QSizeF(image.size()).scaled(box.size(), Qt::KeepAspectRatio);
        target = QRectF((box.width() - size.width()) / 2, (box.height() - size.height()) / 2,
            size.width(), size.height());
    } else if (fit == "cover") {
        const double scale = qMax(box.width() / image.width(), box.height() / image.height());
        const QSizeF visible(box.width() / scale, box.height() / scale);
        source = QRectF((image.width() - visible.width()) / 2,
            (image.height() - visible.height()) / 2, visible.width(), visible.height());
    }
    painter.drawImage(target, image, source);
}
}

QByteArray RemotePreview::render(const QVariantMap &slide, const QVariantList &assets) {
    QImage frame(720, 405, QImage::Format_RGB32);
    frame.fill(QColor(slide.value("background", "#111111").toString()));
    QPainter painter(&frame);
    painter.setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.scale(720.0 / 1920, 405.0 / 1080);
    const QImage ambient(":/qt/qml/ChurchDeck/assets/ambient.svg");
    painter.setOpacity(slide.value("ambientOpacity", 0.5).toDouble());
    painter.drawImage(QRectF(0, 0, 1920, 1080), ambient);
    painter.setOpacity(1);
    for (const auto &value : slide["elements"].toList()) {
        const auto element = value.toMap();
        if (!element.value("visible", true).toBool()) {
            continue;
        }
        const double width = element["width"].toDouble(), height = element["height"].toDouble();
        painter.save();
        painter.translate(
            element["x"].toDouble() + width / 2, element["y"].toDouble() + height / 2);
        painter.rotate(element["rotation"].toDouble());
        painter.translate(-width / 2, -height / 2);
        painter.setClipRect(QRectF(0, 0, width, height));
        painter.setOpacity(qBound(0.0, element.value("opacity", 1).toDouble(), 1.0));
        const auto kind = element.value("kind", "text").toString();
        if (kind == "text") {
            QFont font(element.value("fontFamily", "Arial").toString());
            font.setPixelSize(element.value("fontSize", 36).toInt());
            QTextDocument document;
            document.setDocumentMargin(0);
            document.setDefaultFont(font);
            QTextOption option;
            option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            const auto align = element["alignment"].toString();
            option.setAlignment(align == "center" ? Qt::AlignHCenter
                    : align == "right"            ? Qt::AlignRight
                                                  : Qt::AlignLeft);
            document.setDefaultTextOption(option);
            document.setPlainText(element["text"].toString());
            document.setTextWidth(width);
            QAbstractTextDocumentLayout::PaintContext context;
            context.palette.setColor(
                QPalette::Text, QColor(element.value("color", "#FFFFFF").toString()));
            context.clip = QRectF(0, 0, width, height);
            document.documentLayout()->draw(&painter, context);
        } else if (kind != "audio") {
            QUrl source(element["source"].toString());
            if (kind == "video") {
                const auto original = source.toString();
                source = QUrl();
                for (const auto &asset : assets) {
                    if (asset.toMap()["source"] == original) {
                        source = QUrl(asset.toMap()["thumbnail"].toString());
                        break;
                    }
                }
            }
            const auto image = readImage(source);
            if (!image.isNull()) {
                drawImage(painter, image, QRectF(0, 0, width, height),
                    element.value("fit", "contain").toString());
            } else {
                painter.fillRect(QRectF(0, 0, width, height), QColor("#24282E"));
                painter.setPen(QColor("#BBBBBB"));
                QFont font("Arial");
                font.setPixelSize(36);
                painter.setFont(font);
                painter.drawText(QRectF(0, 0, width, height), Qt::AlignCenter,
                    kind == "video" ? "Video still" : "Missing image");
            }
        }
        painter.restore();
    }
    painter.end();
    QByteArray result;
    QBuffer buffer(&result);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPEG", 82);
    return result;
}

QString RemotePreview::qrDataUrl(const QString &text) {
    const auto qr
        = qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    const int border = 4, scale = 6, size = qr.getSize();
    QImage image((size + border * 2) * scale, (size + border * 2) * scale, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (qr.getModule(x, y)) {
                painter.drawRect((x + border) * scale, (y + border) * scale, scale, scale);
            }
        }
    }
    painter.end();
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return "data:image/png;base64," + QString::fromLatin1(bytes.toBase64());
}
