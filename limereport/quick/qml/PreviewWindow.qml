import QtQuick
import QtQuick.Controls
import LimeReport

ApplicationWindow {
    id: window
    required property ReportPreviewController controller
    property int hints: 0

    // LimeReport::PreviewHint (PreviewBarsUserSetting = 8 keeps everything visible)
    readonly property bool userSetting: (hints & 8) !== 0
    readonly property bool hideToolBar: !userSetting && (hints & 1) !== 0
    readonly property bool hideStatusBar: !userSetting && (hints & 4) !== 0

    width: 1000
    height: 800
    visible: false
    title: controller && controller.title.length > 0 ? controller.title : qsTr("Preview")

    function reloadPreview() { controller.render() }

    ReportPreview {
        anchors.fill: parent
        controller: window.controller
        toolBarVisible: !window.hideToolBar
        statusBarVisible: !window.hideStatusBar
        onCloseRequested: window.close()
    }

    Shortcut { sequences: [StandardKey.Close]; onActivated: window.close() }
    Shortcut { sequence: "Escape"; onActivated: window.close() }
}
