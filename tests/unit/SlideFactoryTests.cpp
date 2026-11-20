#include "SlideFactory.h"
#include <QDebug>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QTextLayout>
#include <QUuid>

int SlideFactory::selfTest() {
    QString error;
    QVariantMap song { { "id", "demo" }, { "title", "Gathering" },
        { "sections",
            QVariantList { QVariantMap { { "key", "v1" }, { "label", "Verse 1" },
                { "primary", "One\nTwo\nThree\nFour" },
                { "secondary", "ఒకటి\nరెండు\nమూడు\nనాలుగు" } } } },
        { "arrangement", QVariantList { "v1", "v1" } } };
    const auto pages = songSlides(song, 2, &error);
    if (!error.isEmpty() || pages.size() != 4) {
        return 1;
    }
    QStringList primary, secondary;
    for (const auto &page : pages) {
        const auto elements = page.toMap()["elements"].toList();
        primary << elements[1].toMap()["text"].toString();
        secondary << elements[2].toMap()["text"].toString();
    }
    if (primary.join('\n') != "One\nTwo\nThree\nFour\nOne\nTwo\nThree\nFour"
        || !secondary.join('\n').contains("నాలుగు")) {
        return 2;
    }
    const auto longText = QString("ప్రభువు మనకు ఆశను ఇస్తాడు. ").repeated(80);
    auto verses = scriptureSlides({ QVariantMap { { "text", longText }, { "reference", "Test 1:1" },
                                      { "translation", "test" }, { "abbreviation", "TEST" } } },
        3, &error);
    QString assembled;
    for (const auto &page : verses) {
        assembled += page.toMap()["elements"].toList()[1].toMap()["text"].toString();
    }
    if (verses.size() < 2
        || assembled.simplified().remove(' ') != longText.simplified().remove(' ')) {
        return 3;
    }
    qInfo() << "Slide generation checks passed: aligned bilingual pages, repeated arrangements, "
               "and lossless long-scripture splitting.";
    return 0;
}
