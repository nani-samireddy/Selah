#pragma once
#include <QJsonObject>
#include <QUrl>
class QIODevice;

// Converts Beblia and Zefania XML into the same validated package used by local JSON imports.
namespace BibleXmlReader {
QJsonObject read(QIODevice &input, const QUrl &source, QString *error);
}
