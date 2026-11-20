#include "ProgramSlides.h"
#include "ProgramSources.h"
#include "ProgramStore.h"
#include "SlideFactory.h"
#include "TemplateDefinition.h"

QString ProgramStore::layoutKind() const {
    const auto item = selectedItem();
    if (item["kind"] == "scripture") {
        return "scripture";
    }
    if (item["kind"] != "song") {
        return {};
    }
    for (const auto &value : item["song"].toMap()["sections"].toList()) {
        if (!value.toMap()["secondary"].toString().trimmed().isEmpty()) {
            return "bilingual";
        }
    }
    return "song";
}

QVariantMap ProgramStore::previewLayout(const QVariantMap &definition) const {
    const auto item = selectedItem();
    QString error;
    QVariantList slides;
    const int lines = item.value("maxLines", 4).toInt();
    if (item["kind"] == "song") {
        slides = SlideFactory::songSlides(item["song"].toMap(), lines, &error, definition);
    } else if (item["kind"] == "scripture") {
        const auto verses = ProgramSources::scripture(item, &error);
        if (error.isEmpty()) {
            slides = SlideFactory::scriptureSlides(verses, lines, &error, definition);
        }
    } else {
        error = "Choose a song or Bible passage to apply a content template";
    }
    if (error.isEmpty()
        && flatten().size() - item["slides"].toList().size() + slides.size() > 2000) {
        error = "This layout would exceed 2,000 program slides";
    }
    if (!error.isEmpty()) {
        slides.clear();
    }
    return { { "slides", slides }, { "error", error },
        { "missingMedia", TemplateDefinition::missingMedia(definition) } };
}

bool ProgramStore::applyLayout(const QVariantMap &definition) {
    if (definition.isEmpty()) {
        statusMessage("Choose a layout template");
        return false;
    }
    const auto preview = previewLayout(definition);
    if (!preview["error"].toString().isEmpty() || preview["slides"].toList().isEmpty()) {
        statusMessage(preview["error"].toString());
        return false;
    }
    auto item = selectedItem();
    checkpoint();
    item["layoutTemplate"] = definition;
    if (item["kind"] == "scripture" && item["verses"].toList().isEmpty()) {
        QString error;
        item["verses"] = ProgramSources::scripture(item, &error);
    }
    item["slides"] = ProgramSlides::cloneWithFreshIds(preview["slides"].toList());
    item["template"] = "original";
    replaceItem(item);
    return true;
}

bool ProgramStore::clearLayout() {
    const auto preview = previewLayout({});
    if (!preview["error"].toString().isEmpty()) {
        statusMessage(preview["error"].toString());
        return false;
    }
    auto item = selectedItem();
    checkpoint();
    item.remove("layoutTemplate");
    item["slides"] = ProgramSlides::cloneWithFreshIds(preview["slides"].toList());
    replaceItem(item);
    return true;
}
