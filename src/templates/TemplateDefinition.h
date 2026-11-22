#pragma once
#include <QStringList>
#include <QVariantMap>

// A definition is a named, typed slide layout. Bound text boxes define pagination constraints.
namespace TemplateDefinition {
QStringList bindings();
QVariantMap starter(const QString &kind);
QVariantMap dualScripture();
bool hasVisibleBinding(const QVariantMap &definition, const QString &binding);
bool validate(const QVariantMap &definition, QString *error);
bool supports(const QVariantMap &definition, const QString &kind, bool bilingual, QString *error);
QStringList missingMedia(const QVariantMap &definition);
}
