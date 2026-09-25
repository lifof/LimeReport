import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Creates or edits an SQL query, SQL sub-query or CSV datasource.
Dialog {
    id: dialog
    property var designer: null
    property string oldName: ""
    signal failed(string message)

    readonly property string type: typeBox.currentValue

    function add() { oldName = ""; load({ type: "query" }); open() }
    function edit(name) { oldName = name; load(designer.datasourceInfo(name)); open() }
    function load(info) {
        nameField.text = info.name || ""
        typeBox.currentIndex = Math.max(0, typeBox.indexOfValue(info.type || "query"))
        connectionBox.currentIndex = Math.max(0, connectionBox.find(info.connection || ""))
        masterBox.currentIndex = Math.max(0, masterBox.find(info.master || ""))
        sqlArea.text = info.sql || ""
        csvArea.text = info.csv || ""
        separatorField.text = info.separator || ";"
        headerBox.checked = info.firstRowIsHeader || false
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: Math.min(parent.width * 0.9, 700)
    height: Math.min(parent.height * 0.9, 520)
    title: oldName.length > 0 ? qsTr("Edit datasource") : qsTr("New datasource")
    standardButtons: Dialog.Ok | Dialog.Cancel

    ColumnLayout {
        anchors.fill: parent
        GridLayout {
            columns: 2
            Layout.fillWidth: true
            Label { text: qsTr("Datasource name") }
            TextField { id: nameField; Layout.fillWidth: true }
            Label { text: qsTr("Type") }
            ComboBox {
                id: typeBox
                Layout.fillWidth: true
                textRole: "text"
                valueRole: "value"
                model: [
                    { text: qsTr("SQL query"), value: "query" },
                    { text: qsTr("SQL sub-query (master/detail)"), value: "subquery" },
                    { text: qsTr("CSV"), value: "csv" }
                ]
            }
            Label { text: qsTr("Connection"); visible: dialog.type !== "csv" }
            ComboBox {
                id: connectionBox
                Layout.fillWidth: true
                visible: dialog.type !== "csv"
                model: dialog.designer ? dialog.designer.connectionNames : []
            }
            Label { text: qsTr("Master datasource"); visible: dialog.type === "subquery" }
            ComboBox {
                id: masterBox
                Layout.fillWidth: true
                visible: dialog.type === "subquery"
                model: dialog.designer ? dialog.designer.datasourceNames : []
            }
            Label { text: qsTr("Separator"); visible: dialog.type === "csv" }
            RowLayout {
                visible: dialog.type === "csv"
                TextField { id: separatorField; implicitWidth: 60 }
                CheckBox { id: headerBox; text: qsTr("First row is header") }
            }
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: dialog.type !== "csv"
            TextArea { id: sqlArea; font.family: "monospace"; placeholderText: "SELECT * FROM ..."; selectByMouse: true }
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: dialog.type === "csv"
            TextArea { id: csvArea; font.family: "monospace"; selectByMouse: true }
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.type === "subquery"
            opacity: 0.7
            wrapMode: Text.Wrap
            text: qsTr("Refer to master fields with $D{field} and to variables with $V{name}.")
        }
    }

    onAccepted: {
        var info = {
            name: nameField.text, type: type, connection: connectionBox.currentText,
            master: masterBox.currentText, sql: sqlArea.text, csv: csvArea.text,
            separator: separatorField.text, firstRowIsHeader: headerBox.checked
        }
        if (!designer.saveDatasource(info, oldName)) failed(designer.lastError)
    }
}
