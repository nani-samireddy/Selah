pragma ComponentBehavior: Bound
import QtQuick

LibraryCombo {
    id: picker

    property string value: "English"
    property bool optional: false
    property var recent: []

    // Imported language names remain selectable even when outside the built-in list.
    readonly property var languages: ["English", "Assamese", "Bengali", "Bodo", "Dogri", "Gujarati",
        "Hindi", "Kannada", "Kashmiri", "Konkani", "Maithili", "Malayalam", "Manipuri", "Marathi",
        "Nepali", "Odia", "Punjabi", "Sanskrit", "Santali", "Sindhi", "Tamil", "Telugu", "Urdu"]

    signal chosen

    model: {
        let names = [];
        for (const name of recent.concat(languages, value ? [value] : []))
            if (name && !names.includes(name))
                names.push(name);
        let rows = names.map(name => ({
            label: name,
            value: name
        }));
        return optional ? [
                              {
                                  label: "None (single language)",
                                  value: ""
                              }
                          ].concat(rows) : rows;
    }
    textRole: "label"
    valueRole: "value"
    currentIndex: model.findIndex(row => row.value === value)
    Accessible.name: optional ? "Second language (optional)" : "Primary language"

    onActivated: {
        value = currentValue;
        chosen();
    }
}
