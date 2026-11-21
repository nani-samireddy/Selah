#pragma once
#include <QQuickItem>
#include <QString>

namespace VisualLookup {
// Repeater delegates follow visual parenting, not necessarily QObject ownership.
inline QQuickItem *find(QQuickItem *root, const QString &name) {
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    for (auto *child : root->childItems()) {
        if (auto *result = find(child, name)) {
            return result;
        }
    }
    return nullptr;
}
}
