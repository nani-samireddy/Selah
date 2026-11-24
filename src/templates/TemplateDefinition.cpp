#include "TemplateDefinition.h"
#include "DocumentElements.h"
#include "EditorDocument.h"
#include <QFileInfo>
#include <QSet>
#include <QTransform>
#include <QUrl>

namespace {
QVariantMap textBox(
    const QString &name, const QString &text, int y, int height, int size, const QString &binding) {
    auto element = DocumentElements::text(name, text, 160, y, 1600, height, size, "Arial");
    element["alignment"] = "center";
    element["binding"] = binding;
    return element;
}
}

QStringList TemplateDefinition::bindings() {
    return { "None", "Title", "Section name", "Primary lyrics", "Translated lyrics",
        "Scripture text", "Scripture reference", "Credits", "Version", "Secondary scripture text",
        "Secondary scripture reference", "Secondary version", "Secondary credits" };
}

QVariantMap TemplateDefinition::starter(const QString &kind) {
    if (kind == "presentation") {
        auto title = textBox("Heading", "Your message", 280, 260, 120, "None");
        return { { "kind", kind }, { "name", "Presentation" },
            { "slide",
                QVariantMap { { "name", "Template" }, { "background", "#252A3A" },
                    { "ambientOpacity", 0.0 }, { "elements", QVariantList { title } } } } };
    }
    const bool bilingual = kind == "bilingual";
    const bool scripture = kind == "scripture";
    QVariantList elements;
    elements << textBox(
        "Title", scripture ? "Scripture reading" : "Song title", 65, 70, 28, "Title");
    elements << textBox("Primary content", scripture ? "Scripture text" : "Primary lyrics",
        bilingual ? 190 : 235, bilingual ? 375 : 650, 96,
        scripture ? "Scripture text" : "Primary lyrics");
    if (bilingual) {
        auto translated
            = textBox("Translation", "అనువాద పంక్తులు", 615, 270, 64, "Translated lyrics");
        translated["fontFamily"] = "Noto Sans Telugu";
        elements << translated;
    }
    elements << textBox(scripture ? "Reference" : "Credits",
        scripture ? "Reference" : "Author / copyright", 990, 60, 24,
        scripture ? "Scripture reference" : "Credits");
    if (scripture) {
        auto version = textBox("Version", "Version", 80, 55, 22, "Version");
        version["x"] = 1510;
        version["width"] = 250;
        elements << version;
    }
    return { { "kind", kind },
        { "name",
            scripture       ? "Scripture"
                : bilingual ? "Bilingual lyrics"
                            : "Lyrics" },
        { "slide",
            QVariantMap { { "name", "Template" }, { "background", "#252A3A" },
                { "ambientOpacity", 0.0 }, { "notes", "" }, { "elements", elements } } } };
}

bool TemplateDefinition::validate(const QVariantMap &definition, QString *error) {
    if (error) {
        error->clear();
    }
    const auto fail = [error](const QString &message) {
        if (error) {
            *error = message;
        }
        return false;
    };
    const auto kind = definition["kind"].toString();
    if (!QStringList { "song", "bilingual", "scripture", "presentation" }.contains(kind)) {
        return fail("Choose a presentation, lyrics, bilingual lyrics, or scripture template");
    }
    if (definition["name"].toString().trimmed().isEmpty()
        || definition["name"].toString().size() > 200) {
        return fail("Enter a template name of up to 200 characters");
    }
    const auto slide = definition["slide"].toMap();
    if (EditorDocument::validatedSlides({ slide }).isEmpty()) {
        return fail("The template contains invalid slide elements");
    }
    if (slide["elements"].toList().size() > 100) {
        return fail("A template can contain up to 100 layers");
    }
    QSet<QString> visible;
    for (const auto &value : slide["elements"].toList()) {
        const auto box = value.toMap();
        const auto binding = box.value("binding", "None").toString();
        if (binding == "None" || binding.isEmpty()) {
            continue;
        }
        if (!bindings().contains(binding) || box["kind"] != "text") {
            return fail("Content bindings must use a supported text layer");
        }
        if (kind == "presentation") {
            return fail("Presentation templates use editable static text. Remove content bindings "
                        "or choose a lyrics/scripture template.");
        }
        if (box["fontFamily"].toString().trimmed().isEmpty()) {
            return fail("Choose a font for every bound text box");
        }
        const bool scriptureBinding = binding == "Scripture text"
            || binding == "Scripture reference" || binding == "Version"
            || binding.startsWith("Secondary ");
        const bool songBinding = binding == "Primary lyrics" || binding == "Translated lyrics"
            || binding == "Section name";
        if ((kind == "scripture" && songBinding) || (kind != "scripture" && scriptureBinding)) {
            return fail("The content binding does not match this template's kind");
        }
        if (!box.value("visible", true).toBool() || box.value("opacity", 1).toDouble() <= 0) {
            continue;
        }
        QTransform transform;
        const double width = box["width"].toDouble(), height = box["height"].toDouble();
        transform.translate(box["x"].toDouble() + width / 2, box["y"].toDouble() + height / 2);
        transform.rotate(box["rotation"].toDouble());
        transform.translate(-width / 2, -height / 2);
        if (!QRectF(0, 0, 1920, 1080).contains(transform.mapRect(QRectF(0, 0, width, height)))) {
            return fail("Keep bound text boxes inside the slide, including rotated corners");
        }
        visible.insert(binding);
    }
    if (kind != "presentation"
        && !visible.contains(kind == "scripture" ? "Scripture text" : "Primary lyrics")) {
        return fail(kind == "scripture" ? "Bind a visible text box to Scripture text"
                                        : "Bind a visible text box to Primary lyrics");
    }
    if (kind == "bilingual" && !visible.contains("Translated lyrics")) {
        return fail("A bilingual template needs a visible Translated lyrics box");
    }
    if (kind == "song" && visible.contains("Translated lyrics")) {
        return fail("Choose the bilingual kind to include translated lyrics");
    }
    return true;
}

bool TemplateDefinition::supports(
    const QVariantMap &definition, const QString &kind, bool bilingual, QString *error) {
    if (!validate(definition, error)) {
        return false;
    }
    const auto type = definition["kind"].toString();
    if (type == "presentation" || (kind == "scripture" && type != "scripture")
        || (kind == "song" && type == "scripture") || (bilingual && type != "bilingual")) {
        if (error) {
            *error = bilingual ? "Choose a bilingual template to preserve both languages"
                               : "Choose a template that matches this content";
        }
        return false;
    }
    return true;
}

QStringList TemplateDefinition::missingMedia(const QVariantMap &definition) {
    QSet<QString> missing;
    const auto fill = definition["slide"].toMap().value("backgroundFill").toMap();
    if (fill.value("type") == "image") {
        const auto path = QUrl(fill.value("source").toString()).toLocalFile();
        if (!QFileInfo::exists(path)) {
            missing.insert(QFileInfo(path).fileName());
        }
    }
    for (const auto &value : definition["slide"].toMap()["elements"].toList()) {
        const auto element = value.toMap();
        if (element["kind"] == "image" || element["kind"] == "video") {
            const auto path = QUrl(element["source"].toString()).toLocalFile();
            if (!QFileInfo::exists(path)) {
                missing.insert(QFileInfo(path).fileName());
            }
        }
    }
    return missing.values();
}

bool TemplateDefinition::hasVisibleBinding(const QVariantMap &definition, const QString &binding) {
    for (const auto &value : definition["slide"].toMap()["elements"].toList()) {
        const auto box = value.toMap();
        if (box["binding"] == binding && box.value("visible", true).toBool()
            && box.value("opacity", 1).toDouble() > 0) {
            return true;
        }
    }
    return false;
}

QVariantMap TemplateDefinition::dualScripture() {
    auto definition = starter("scripture");
    definition["name"] = "Dual Bible";
    QVariantList elements;
    auto primary = textBox("First Bible", "Scripture text", 160, 330, 64, "Scripture text");
    auto secondary
        = textBox("Second Bible", "రెండవ బైబిలు వచనం", 620, 330, 64, "Secondary scripture text");
    secondary["fontFamily"] = "Noto Sans Telugu";
    elements << textBox("Reference", "Reference", 65, 55, 28, "Scripture reference")
             << textBox("Version", "Version", 120, 35, 22, "Version") << primary
             << textBox(
                    "Second reference", "Reference", 520, 55, 28, "Secondary scripture reference")
             << textBox("Second version", "Version", 575, 35, 22, "Secondary version") << secondary
             << textBox("Credits", "", 990, 30, 14, "Credits")
             << textBox("Second credits", "", 1030, 30, 14, "Secondary credits");
    auto slide = definition["slide"].toMap();
    slide["elements"] = elements;
    slide["ambientOpacity"] = 0.0;
    definition["slide"] = slide;
    return definition;
}
