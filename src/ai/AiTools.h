#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QString>
#include <QVariantList>

namespace AiTools {
// The model proposes searches. It has no write, filesystem or presentation tools.
QJsonArray schemas(const QString &scope = "all");

struct Proposal {
    QVariantList actions;
    QString error;
};

Proposal decode(const QByteArray &response);
}
