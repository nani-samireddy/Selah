#pragma once
#include <QStringList>
#include <QVariantList>

// Translation-independent reference resolution. No verse text is generated or modified.
namespace BookMatcher {
struct Result {
    bool handled = false;
    QString query, code, error;
    QVariantList choices;
};

QString normalize(const QString &text);
QString romanize(const QString &text);
Result resolve(
    const QString &text, const QVariantList &knownBooks, const QVariantList &availableBooks);
}
