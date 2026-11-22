pragma ComponentBehavior: Bound
import QtQuick

LibraryCombo {
    id: selector

    required property var library
    property string kind: "song"
    property string selectedId: ""
    property bool dualScripture: false
    readonly property var records: library.layouts
    readonly property var definition: records.find(record => record.id === selectedId) || ({})

    function supportsDual(record) {
        return (record.slide?.elements || []).some(box => box.binding
                                                          === "Secondary scripture text"
                                                          && box.visible !== false && (box.opacity
                                                                                       === undefined
                                                                                       || box.opacity
                                                                                       > 0));
    }

    model: [
        {
            id: "",
            name: "Automatic layout"
        }
    ].concat(records.filter(record => kind === "scripture" ? record.kind === "scripture" && (
                                                                 !dualScripture || supportsDual(
                                                                     record)) : kind
                                                             === "bilingual" ? record.kind
                                                                               === "bilingual" : (
                                                                                   record.kind
                                                                                   === "song"
                                                                                   || record.kind
                                                                                   === "bilingual")))
    textRole: "name"
    currentIndex: Math.max(0, model.findIndex(record => record.id === selectedId))

    onDualScriptureChanged: selectedId = ""
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
