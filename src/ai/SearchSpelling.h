#pragma once
#include <QString>
#include <QStringList>

namespace SearchSpelling {
// Correct only a unique nearby word. Ambiguous and short words stay as typed.
enum class Vocabulary { Songs, BibleBooks };
QString correct(
    const QString &query, const QStringList &vocabulary, Vocabulary kind = Vocabulary::Songs);
QStringList words(const QString &text);
}
