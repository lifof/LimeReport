import QtQuick
import QtQuick.Controls
import LimeReport

// Scrollable, zoomable design surface for one report page.
Item {
    id: root
    property alias scene: view.scene
    property alias zoom: view.zoom
    property alias view: view
    property var designer: null

    function zoomIn() { view.zoom = view.zoom * 1.2 }
    function zoomOut() { view.zoom = view.zoom / 1.2 }
    function zoomTo(percent) { view.zoom = percent / 100 * 0.378 }
    function fitWidth() { view.fitWidth() }

    ReportSceneView {
        id: view
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: vbar.left
        anchors.bottom: hbar.top
        interactive: true
        backgroundColor: palette.mid
        zoom: 0.5
        focus: true
        onPopupMenuRequested: function(menu, x, y) { popup.show(menu, x, y) }
    }

    DropArea {
        anchors.fill: view
        keys: ["text/plain"]
        function dragText(drag) {
            if (drag.hasText) return drag.text
            return drag.source && drag.source.dragText !== undefined ? drag.source.dragText : ""
        }
        onEntered: function(drag) { drag.accepted = view.dragEnter(drag.x, drag.y, dragText(drag)) }
        onPositionChanged: function(drag) { view.dragMove(drag.x, drag.y, dragText(drag)) }
        onExited: view.dragLeave()
        onDropped: function(drop) {
            if (view.drop(drop.x, drop.y, dragText(drop))) drop.acceptProposedAction()
            view.forceActiveFocus()
        }
    }

    ScrollBar {
        id: vbar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: hbar.top
        orientation: Qt.Vertical
        policy: view.contentHeight > view.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        size: view.contentHeight > 0 ? view.height / view.contentHeight : 1
        position: view.contentHeight > 0 ? view.contentY / view.contentHeight : 0
        onPositionChanged: if (pressed) view.contentY = position * view.contentHeight
        width: policy === ScrollBar.AlwaysOn ? implicitWidth : 0
    }
    ScrollBar {
        id: hbar
        anchors.left: parent.left
        anchors.right: vbar.left
        anchors.bottom: parent.bottom
        orientation: Qt.Horizontal
        policy: view.contentWidth > view.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        size: view.contentWidth > 0 ? view.width / view.contentWidth : 1
        position: view.contentWidth > 0 ? view.contentX / view.contentWidth : 0
        onPositionChanged: if (pressed) view.contentX = position * view.contentWidth
        height: policy === ScrollBar.AlwaysOn ? implicitHeight : 0
    }

    PopupMenuHost { id: popup; parent: view }
}
