import QtQuick
import QtQuick.Controls
import LimeReport

ApplicationWindow {
    id: window
    required property QtObject engine
    property bool closeConfirmed: false

    width: 1280
    height: 860
    visible: false
    title: designer.title + " - " + qsTr("Lime Report Designer")

    ReportDesigner {
        id: designer
        anchors.fill: parent
        engine: window.engine
    }

    onClosing: function(close) {
        if (!closeConfirmed && designer.modified) {
            close.accepted = false
            designer.confirmDiscard(function() { window.closeConfirmed = true; window.close() })
        }
    }
}
