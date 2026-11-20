#include "ProgramSlides.h"
#include <QUuid>

QVariantList ProgramSlides::cloneWithFreshIds(QVariantList slides) {
    int index = 0;
    for (auto &s : slides) {
        auto page = s.toMap();
        if (!page.contains("ambientOpacity")) {
            page["ambientOpacity"] = index == 0 ? 0.85 : 0.5;
        }
        ++index;
        auto elements = page["elements"].toList();
        for (auto &e : elements) {
            auto item = e.toMap();
            item["uid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
            e = item;
        }
        page["elements"] = elements;
        s = page;
    }
    return slides;
}
