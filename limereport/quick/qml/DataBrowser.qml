import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Connections, datasources, fields and variables of the report. Fields and
// variables can be dragged onto the design surface.
Pane {
    id: browser
    property var designer: null
    padding: 0

    readonly property var currentEntry: list.currentIndex >= 0 && list.model ? list.model.get(list.currentIndex) : null
    readonly property string currentKind: currentEntry ? currentEntry.kind : ""

    function editCurrent() {
        if (!currentEntry) return
        var k = currentEntry.kind
        if (k === "connection") connectionDialog.edit(currentEntry.name)
        else if (k === "query" || k === "subquery" || k === "csv") datasourceDialog.edit(currentEntry.name)
        else if (k === "variable") variableDialog.edit(currentEntry.name)
    }
    function deleteCurrent() {
        if (!currentEntry) return
        confirmDelete.entry = currentEntry
        confirmDelete.open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                spacing: 0
                IconButton { iconName: "databases"; tip: qsTr("Add database connection"); onClicked: connectionDialog.add() }
                IconButton { iconName: "table"; tip: qsTr("Add datasource"); onClicked: datasourceDialog.add() }
                IconButton { iconName: "variable"; tip: qsTr("Add variable"); onClicked: variableDialog.add() }
                IconButton {
                    iconName: "edit"; tip: qsTr("Edit")
                    enabled: ["connection", "query", "subquery", "csv", "variable"].indexOf(browser.currentKind) >= 0
                    onClicked: browser.editCurrent()
                }
                IconButton {
                    iconName: "delete"; tip: qsTr("Delete")
                    enabled: ["connection", "query", "subquery", "csv", "proxy", "variable"].indexOf(browser.currentKind) >= 0
                    onClicked: browser.deleteCurrent()
                }
                IconButton {
                    iconName: "database"; tip: qsTr("Connect / disconnect")
                    enabled: browser.currentKind === "connection"
                    onClicked: if (!browser.designer.toggleConnection(browser.currentEntry.name)) errorDialog.show(browser.designer.lastError)
                }
                IconButton { text: "⟳"; tip: qsTr("Refresh"); onClicked: browser.designer.refreshData() }
            }
        }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: browser.designer ? browser.designer.dataModel : null
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            delegate: TreeRow {
                id: row
                width: list.width
                treeModel: list.model
                current: ListView.isCurrentItem
                iconName: kind === "connection" ? (status === "connected" ? "database" : "databases")
                        : kind === "field" ? "field"
                        : kind.indexOf("ariable") >= 0 ? "value"
                        : kind === "category" ? "folder"
                        : "table"
                onActivated: list.currentIndex = index
                onDoubleClickedRow: { list.currentIndex = index; browser.editCurrent() }

                Drag.active: dragHandler.active
                Drag.dragType: Drag.Automatic
                Drag.supportedActions: Qt.CopyAction
                Drag.mimeData: { "text/plain": row.dragText }
                Drag.keys: ["text/plain"]
                DragHandler {
                    id: dragHandler
                    enabled: row.dragText.length > 0
                    target: null
                }
            }
        }
    }

    ConnectionDialog { id: connectionDialog; designer: browser.designer; onFailed: function(message) { errorDialog.show(message) } }
    DataSourceDialog { id: datasourceDialog; designer: browser.designer; onFailed: function(message) { errorDialog.show(message) } }
    VariableDialog { id: variableDialog; designer: browser.designer; onFailed: function(message) { errorDialog.show(message) } }

    Dialog {
        id: confirmDelete
        property var entry: null
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Attention")
        standardButtons: Dialog.Yes | Dialog.No
        Label { text: confirmDelete.entry ? qsTr("Do you really want to delete \"%1\"?").arg(confirmDelete.entry.name) : "" }
        onAccepted: {
            var e = entry
            if (e.kind === "connection") browser.designer.deleteConnection(e.name)
            else if (e.kind === "variable") browser.designer.deleteVariable(e.name)
            else browser.designer.deleteDatasource(e.name)
        }
    }
    Dialog {
        id: errorDialog
        function show(message) { errorText.text = message; open() }
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Error")
        standardButtons: Dialog.Ok
        Label { id: errorText; wrapMode: Text.Wrap; width: Math.min(implicitWidth, 400) }
    }
}
