import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Row delegate for the flattened tree models (object tree, data browser).
ItemDelegate {
    id: row
    required property int index
    required property string label
    required property string kind
    required property int depth
    required property bool hasChildren
    required property bool expanded
    required property string dragText
    required property string status
    required property bool selected
    required property var object

    property var treeModel
    property string iconName: ""
    property bool current: false

    signal activated()
    signal doubleClickedRow()

    height: 24
    highlighted: current || selected
    padding: 0

    contentItem: RowLayout {
        spacing: 4
        Item { implicitWidth: row.depth * 14 + 4 }
        Label {
            Layout.preferredWidth: 12
            text: row.hasChildren ? (row.expanded ? "▾" : "▸") : ""
            MouseArea {
                anchors.fill: parent
                anchors.margins: -4
                onClicked: row.treeModel.toggle(row.index)
            }
        }
        Image {
            visible: row.iconName.length > 0
            source: row.iconName.length > 0 ? "qrc:/report/images/" + row.iconName : ""
            sourceSize.width: 16
            sourceSize.height: 16
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
        }
        Label {
            Layout.fillWidth: true
            text: row.label
            elide: Text.ElideRight
            color: row.status === "error" ? "firebrick" : palette.text
            font.bold: row.kind === "category"
        }
    }

    onClicked: activated()
    onDoubleClicked: { if (hasChildren && dragText.length === 0) treeModel.toggle(index); doubleClickedRow() }
}
