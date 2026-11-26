pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import ChurchDeck.Formatting

// A single stable editor survives model updates while the inspector formats a selection.
TextEdit {
    id: editor

    required property var host
    property var textElement: ({})
    property bool settingText: false
    property bool committing: false
    property int selectionFrom: 0
    property int selectionTo: 0
    property var style: ({})
    readonly property bool sessionOpen: host.editingId.length > 0
    readonly property real fitScale: textElement.textSizing === "shrink" ? (TextFormatting.display(
                                                                                textElement).fontSize
                                                                            || 36) / (
                                                                               textElement.fontSize
                                                                               || 36) : 1

    function begin(index) {
        finish();
        host.doc.selected = index;
        textElement = Object.assign({}, host.doc.selectedElement);
        settingText = true;
        text = TextFormatting.html(textElement);
        TextFormatting.hydrate(textDocument, textElement);
        settingText = false;
        host.editingId = textElement.uid;
        host.activeTextEditor = editor;
        forceActiveFocus();
        selectAll();
        rememberSelection();
    }

    function currentElement() {
        return TextFormatting.captureDocument(textElement, textDocument);
    }

    function rememberSelection() {
        if (!sessionOpen || settingText)
            return;
        selectionFrom = selectionStart;
        selectionTo = selectionEnd;
        style = TextFormatting.styleAt(currentElement(), selectionFrom, selectionTo);
    }

    function commit() {
        if (committing)
            return;
        debounce.stop();
        if (!sessionOpen || host.doc.selectedElement.uid !== textElement.uid)
            return;
        committing = true;
        textElement = currentElement();
        host.doc.updateElement(host.doc.selected, textElement);
        rememberSelection();
        committing = false;
    }

    function applyFormat(patch) {
        if (!sessionOpen || host.doc.selectedElement.uid !== textElement.uid)
            return;
        debounce.stop();
        const from = selectionFrom, to = selectionTo;
        textElement = TextFormatting.apply(currentElement(), patch, from, to);
        settingText = true;
        text = textElement.richText;
        TextFormatting.hydrate(textDocument, textElement);
        settingText = false;
        // Store Qt's canonical document form now, so finishing the edit does not
        // add a second history entry for the same visible formatting change.
        textElement = currentElement();
        host.doc.updateElement(host.doc.selected, textElement);
        select(from, to);
        rememberSelection();
    }

    function finish() {
        if (!sessionOpen)
            return;
        commit();
        host.editingId = "";
        host.activeTextEditor = null;
    }

    objectName: "richTextEditor"
    x: (textElement.x || 0) - (width - (textElement.width || 400)) / 2
    y: (textElement.y || 0) - (height - (textElement.height || 200)) / 2
    width: (textElement.width || 400) / fitScale
    height: (textElement.height || 200) / fitScale
    scale: fitScale
    rotation: textElement.rotation || 0
    opacity: textElement.opacity === undefined ? 1 : textElement.opacity
    visible: sessionOpen
    readOnly: !sessionOpen
    textFormat: TextEdit.RichText
    selectByMouse: true
    persistentSelection: true
    selectByKeyboard: true
    selectionColor: "#7F4B93"
    selectedTextColor: "#FFFFFF"
    wrapMode: textElement.wrap === false ? TextEdit.NoWrap : TextEdit.Wrap
    clip: textElement.clipText !== false
    verticalAlignment: textElement.verticalAlignment === "center" ? TextEdit.AlignVCenter :
                                                                    textElement.verticalAlignment
                                                                    === "bottom"
                                                                    ? TextEdit.AlignBottom :
                                                                      TextEdit.AlignTop
    font.family: textElement.fontFamily || "Arial"
    font.pixelSize: textElement.fontSize || 36

    onSelectionStartChanged: rememberSelection()
    onSelectionEndChanged: rememberSelection()
    onTextChanged: if (!settingText && !committing && sessionOpen)
                       commit()
    // Canvas text shares the document's formatting history; TextEdit's own
    // history can otherwise consume Cmd/Ctrl+Z after programmatic formatting.
    Keys.onShortcutOverride: event => {
        if (event.matches(StandardKey.Undo) || event.matches(StandardKey.Redo))
            event.accepted = true;
    }
    Keys.onPressed: event => {
        if (event.matches(StandardKey.Undo)) {
            event.accepted = true;
            host.undoEditor();
        } else if (event.matches(StandardKey.Redo)) {
            event.accepted = true;
            host.redoEditor();
        } else
            event.accepted = false;
    }
    Keys.onEscapePressed: {
        finish();
        host.contentItem.forceActiveFocus();
    }

    Shortcut {
        sequences: [StandardKey.Bold]
        enabled: editor.sessionOpen && editor.activeFocus

        onActivated: editor.applyFormat({
                                            fontWeight: editor.style.fontWeight >= 600 ? 400 : 700
                                        })
    }

    Shortcut {
        sequences: [StandardKey.Italic]
        enabled: editor.sessionOpen && editor.activeFocus

        onActivated: editor.applyFormat({
                                            italic: !editor.style.italic
                                        })
    }

    Shortcut {
        sequences: [StandardKey.Underline]
        enabled: editor.sessionOpen && editor.activeFocus

        onActivated: editor.applyFormat({
                                            underline: !editor.style.underline
                                        })
    }

    Timer {
        id: debounce

        interval: 400

        onTriggered: editor.commit()
    }

    Connections {
        function onSelectionChanged() {
            if (editor.sessionOpen && editor.host.doc.selectedElement.uid
                    !== editor.textElement.uid)
                editor.finish();
            else if (editor.sessionOpen) {
                let next = Object.assign({}, editor.textElement);
                for (const key of ["x", "y", "width", "height", "rotation", "opacity"])
                    next[key] = editor.host.doc.selectedElement[key];
                editor.textElement = next;
            }
        }

        target: editor.host.doc
    }
}
