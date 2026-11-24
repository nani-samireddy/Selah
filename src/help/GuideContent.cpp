#include "GuideContent.h"
#include <QFile>
#include <QStringList>

QString GuideContent::read(const QString &topic) const {
    static const QStringList topics { "getting-started.md", "songs.md", "bible.md", "design.md",
        "presenting.md", "remote-and-data.md" };
    if (!topics.contains(topic)) {
        return {};
    }
    QFile file(":/guides/" + topic);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}
