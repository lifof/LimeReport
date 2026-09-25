import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// Creates or edits a database connection of the report.
Dialog {
    id: dialog
    property var designer: null
    property string oldName: ""
    signal failed(string message)

    function add() { oldName = ""; load({}); open() }
    function edit(name) { oldName = name; load(designer.connectionInfo(name)); open() }
    function load(info) {
        nameField.text = info.name || ""
        defaultBox.checked = info.isDefault || false
        driverBox.currentIndex = Math.max(0, driverBox.find(info.driver || "QSQLITE"))
        databaseField.text = info.databaseName || ""
        hostField.text = info.host || ""
        portField.text = info.port || ""
        userField.text = info.userName || ""
        passwordField.text = info.password || ""
        autoConnectBox.checked = info.autoconnect !== undefined ? info.autoconnect : true
        keepCredentialsBox.checked = info.keepDBCredentials !== undefined ? !info.keepDBCredentials : false
    }
    function info() {
        return {
            name: nameField.text, isDefault: defaultBox.checked, driver: driverBox.currentText,
            databaseName: databaseField.text, host: hostField.text, port: portField.text,
            userName: userField.text, password: passwordField.text,
            autoconnect: autoConnectBox.checked, keepDBCredentials: !keepCredentialsBox.checked
        }
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    title: oldName.length > 0 ? qsTr("Edit connection") : qsTr("New connection")
    standardButtons: Dialog.Ok | Dialog.Cancel

    GridLayout {
        columns: 2
        columnSpacing: 8
        Label { text: qsTr("Connection name") }
        TextField { id: nameField; Layout.fillWidth: true; Layout.minimumWidth: 260; enabled: !defaultBox.checked }
        Item { implicitWidth: 1 }
        CheckBox { id: defaultBox; text: qsTr("Use default application connection") }
        Label { text: qsTr("Driver") }
        ComboBox { id: driverBox; Layout.fillWidth: true; model: dialog.designer ? dialog.designer.sqlDrivers : [] }
        Label { text: qsTr("Database") }
        RowLayout {
            TextField { id: databaseField; Layout.fillWidth: true }
            Button { text: "…"; implicitWidth: 28; onClicked: fileDialog.open() }
        }
        Label { text: qsTr("Server") }
        TextField { id: hostField; Layout.fillWidth: true }
        Label { text: qsTr("Port") }
        TextField { id: portField; Layout.fillWidth: true; validator: IntValidator { bottom: 0; top: 65535 } }
        Label { text: qsTr("User") }
        TextField { id: userField; Layout.fillWidth: true }
        Label { text: qsTr("Password") }
        TextField { id: passwordField; Layout.fillWidth: true; echoMode: TextInput.Password }
        Item { implicitWidth: 1 }
        CheckBox { id: autoConnectBox; text: qsTr("Auto connect") }
        Item { implicitWidth: 1 }
        CheckBox { id: keepCredentialsBox; text: qsTr("Don't keep credentials in the report file") }
        Item { implicitWidth: 1 }
        Button {
            text: qsTr("Check connection")
            onClicked: {
                if (dialog.designer.checkConnection(dialog.info())) status.text = qsTr("Connection successfully established!")
                else status.text = dialog.designer.lastError
            }
        }
        Label { id: status; Layout.columnSpan: 2; Layout.fillWidth: true; wrapMode: Text.Wrap }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Database file")
        onAccepted: {
            var path = selectedFile.toString()
            databaseField.text = path.startsWith("file://") ? decodeURIComponent(path.substring(7)) : path
        }
    }

    onAccepted: {
        if (!designer.saveConnection(info(), oldName)) failed(designer.lastError)
    }
    onOpened: status.text = ""
}
