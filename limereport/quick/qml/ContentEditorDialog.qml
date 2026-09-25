import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Multi-line editor used for item content, scripts and SQL.
Dialog {
    id: dialog
    property alias text: area.text
    property string hint: qsTr("Use $D{datasource.field}, $V{variable} and $S{script} expressions.")

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: Math.min(parent ? parent.width * 0.8 : 700, 700)
    height: Math.min(parent ? parent.height * 0.8 : 500, 500)

    ColumnLayout {
        anchors.fill: parent
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: area
                font.family: "monospace"
                wrapMode: TextEdit.NoWrap
                selectByMouse: true
                focus: true
            }
        }
        Label {
            Layout.fillWidth: true
            text: dialog.hint
            wrapMode: Text.Wrap
            opacity: 0.7
            visible: text.length > 0
        }
    }
}
