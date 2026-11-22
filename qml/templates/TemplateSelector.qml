pragma ComponentBehavior: Bound
import QtQuick

LibraryCombo {
    id: selector

    required property var library
    property string kind: "song"
    property string selectedId: ""
    readonly property var records: library.layouts
    readonly property var definition: records.find(record => record.id === selectedId) || ({})

    model: [
        {
            id: "",
            name: "Automatic layout"
        }
    ].concat(records.filter(record => kind === "scripture" ? record.kind === "scripture" : kind === "bilingual"
                                                             ? record.kind === "bilingual" : (
                                                                   record.kind === "song"
                                                                   || record.kind === "bilingual")))
    textRole: "name"
    currentIndex: Math.max(0, model.findIndex(record => record.id === selectedId))

    onActivated: index => selectedId = model[index].id
    onKindChanged: {
        if (selectedId && (definition.kind === "presentation" || (kind === "scripture"
                                                                  && definition.kind
                                                                  !== "scripture") || (kind
                                                                                       !== "scripture"
                                                                                       && definition.kind
                                                                                       === "scripture")
                           || (kind === "bilingual" && definition.kind !== "bilingual")))
            selectedId = "";
    }
}
