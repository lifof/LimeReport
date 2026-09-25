import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Creates or edits a report variable.
Dialog {
    id: dialog
    property var designer: null
    property string oldName: ""
    signal failed(string message)

    function add() { oldName = ""; load({}); open() }
    function edit(name) { oldName = name; load(designer.variableInfo(name)); open() }
    function load(info) {
        nameField.text = info.name || ""
        valueField.text = info.value || ""
        typeBox.currentIndex = Math.max(0, typeBox.find(info.type || "String"))
        mandatoryBox.checked = info.mandatory || false
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    title: oldName.length > 0 ? qsTr("Edit variable") : qsTr("New variable")
    standardButtons: Dialog.Ok | Dialog.Cancel

    GridLayout {
        columns: 2
        Label { text: qsTr("Variable name") }
        TextField { id: nameField; Layout.fillWidth: true; Layout.minimumWidth: 240 }
        Label { text: qsTr("Type") }
        ComboBox { id: typeBox; Layout.fillWidth: true; model: dialog.designer ? dialog.designer.variableTypes : [] }
        Label { text: qsTr("Value") }
        TextField { id: valueField; Layout.fillWidth: true; placeholderText: typeBox.currentText.indexOf("Date") >= 0 || typeBox.currentText === "Time" ? "ISO 8601" : "" }
        Item { implicitWidth: 1 }
        CheckBox { id: mandatoryBox; text: qsTr("Mandatory") }
    }

    onAccepted: {
        var info = { name: nameField.text, value: valueField.text, type: typeBox.currentText, mandatory: mandatoryBox.checked }
        if (!designer.saveVariable(info, oldName)) failed(designer.lastError)
    }
}
