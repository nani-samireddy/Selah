#include "EditorDocument.h"
#include "PresentationLibrary.h"
#include <QDebug>
#include <QTemporaryDir>

int PresentationLibrary::selfTest() {
    QTemporaryDir directory;
    PresentationLibrary library(nullptr, directory.path());
    EditorDocument document(nullptr, false);
    const auto original = document.slides();
    const auto id = library.save("Sunday announcements", original);
    if (id.isEmpty() || library.presentations().size() != 1
        || library.presentation(id)["slides"].toList() != original) {
        return 1;
    }
    auto changed = original;
    auto first = changed.first().toMap();
    first["name"] = "Updated welcome";
    changed[0] = first;
    if (library.save("Renamed announcements", changed, id) != id
        || library.presentations().size() != 1
        || library.presentation(id)["slides"].toList() != changed) {
        return 2;
    }
    if (!library.save("", changed, id).isEmpty() || !library.save("Invalid", {}, id).isEmpty()
        || !library.save("Missing", changed, "unknown-id").isEmpty()
        || library.presentation(id)["slides"].toList() != changed) {
        return 3;
    }
    PresentationLibrary reopened(nullptr, directory.path());
    if (reopened.presentation(id)["title"] != "Renamed announcements"
        || reopened.presentation(id)["slides"].toList() != changed
        || reopened.save("Separate copy", original).isEmpty()
        || reopened.presentations().size() != 2) {
        return 4;
    }
    qInfo() << "Custom presentation checks passed: save, update, validation, reopen and "
               "independent copies.";
    return 0;
}
