import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import LimeReport

/*
 * Report preview: page navigation, zoom, PDF export, printing (through the
 * platform PDF viewer) and saving of prepared pages.
 *
 *   ReportEngine { id: engine }
 *   ReportPreview {
 *       controller: ReportPreviewController { engine: engine }
 *       Component.onCompleted: { engine.loadFromUrl(url); controller.render() }
 *   }
 */
Item {
    id: root

    property ReportPreviewController controller
    property bool toolBarVisible: true
    property bool statusBarVisible: true
    // 0 = fit width, 1 = fit page, 2 = one to one, 3 = percent (LimeReport::ScaleType)
    property int scaleType: controller ? controller.scaleType : 0
    readonly property alias zoom: view.zoom

    signal closeRequested()

    function fitWidth() {
        scaleType = 0
        view.fitWidth()
    }
    function fitPage() {
        scaleType = 1
        var r = controller ? controller.pageRect(Math.max(1, controller.currentPage)) : Qt.rect(0, 0, 0, 0)
        if (r.width > 0) {
            view.fitRect(Qt.rect(r.x - 10, r.y - 10, r.width + 20, r.height + 20))
        }
    }
    function setPercent(percent) {
        scaleType = percent === 100 ? 2 : 3
        // scene units are 0.1 mm, 1 mm is ~3.78 px at 96 dpi
        view.zoom = percent / 100 * 0.378
    }
    function goToPage(page) {
        if (!controller || controller.pageCount === 0) return
        page = Math.max(1, Math.min(page, controller.pageCount))
        controller.currentPage = page
        var r = controller.pageRect(page)
        view.scrollToScene(view.sceneRect.x, r.y - 10)
    }
    function applyScale() {
        if (!controller || controller.pageCount === 0) return
        if (scaleType === 0) fitWidth()
        else if (scaleType === 1) fitPage()
        else if (scaleType === 2) setPercent(100)
        else setPercent(controller.scalePercent > 0 ? controller.scalePercent : 100)
    }

    Connections {
        target: root.controller
        function onPagesChanged() { Qt.callLater(root.applyScale) }
        function onLastErrorChanged() { errorDialog.text = root.controller.lastError; errorDialog.open() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            visible: root.toolBarVisible
            RowLayout {
                anchors.fill: parent
                spacing: 2
                IconButton {
                    iconName: "print"; tip: qsTr("Print")
                    visible: !root.controller || root.controller.printVisible
                    enabled: root.controller && root.controller.pageCount > 0
                    onClicked: root.controller.print()
                }
                IconButton {
                    iconName: "pdf"; tip: qsTr("Print to PDF")
                    visible: !root.controller || root.controller.printToPdfVisible
                    enabled: root.controller && root.controller.pageCount > 0
                    onClicked: { pdfDialog.currentFile = "file://" + root.controller.defaultFileName("pdf"); pdfDialog.open() }
                }
                IconButton {
                    iconName: "save"; tip: qsTr("Save to file")
                    visible: !root.controller || root.controller.saveToFileVisible
                    enabled: root.controller && root.controller.pageCount > 0
                    onClicked: saveDialog.open()
                }
                IconButton {
                    iconName: "folder"; tip: qsTr("Open prepared pages")
                    visible: !root.controller || root.controller.saveToFileVisible
                    onClicked: openDialog.open()
                }
                IconButton {
                    iconName: "render"; tip: qsTr("Export")
                    visible: root.controller && root.controller.exporters.length > 1
                    enabled: root.controller && root.controller.pageCount > 0
                    onClicked: exportMenu.popup()
                    Menu {
                        id: exportMenu
                        Repeater {
                            model: root.controller ? root.controller.exporters : []
                            MenuItem {
                                text: modelData.description
                                onTriggered: {
                                    exportDialog.exporterName = modelData.name
                                    exportDialog.open()
                                }
                            }
                        }
                    }
                }
                ToolSeparator {}
                IconButton {
                    iconName: "editMode"; tip: qsTr("Edit mode")
                    checkable: true
                    visible: root.controller && root.controller.resultEditable
                    checked: root.controller && root.controller.editMode
                    onToggled: root.controller.editMode = checked
                }
                IconButton {
                    iconName: "addText"; tip: qsTr("Insert text item")
                    visible: root.controller && root.controller.editMode
                    onClicked: root.controller.startInsertTextItem()
                }
                IconButton {
                    iconName: "delete"; tip: qsTr("Delete selected items")
                    visible: root.controller && root.controller.editMode
                    onClicked: root.controller.deleteSelectedItems()
                }
                ToolSeparator { visible: root.controller && root.controller.resultEditable }
                IconButton {
                    iconName: "first"; tip: qsTr("First page")
                    enabled: root.controller && root.controller.currentPage > 1
                    onClicked: root.goToPage(1)
                }
                IconButton {
                    iconName: "prior"; tip: qsTr("Previous page")
                    enabled: root.controller && root.controller.currentPage > 1
                    onClicked: root.goToPage(root.controller.currentPage - 1)
                }
                SpinBox {
                    id: pageSpin
                    from: root.controller && root.controller.pageCount > 0 ? 1 : 0
                    to: root.controller ? root.controller.pageCount : 0
                    value: root.controller ? root.controller.currentPage : 0
                    editable: true
                    onValueModified: root.goToPage(value)
                }
                Label { text: qsTr("of %1").arg(root.controller ? root.controller.pageCount : 0) }
                IconButton {
                    iconName: "next"; tip: qsTr("Next page")
                    enabled: root.controller && root.controller.currentPage < root.controller.pageCount
                    onClicked: root.goToPage(root.controller.currentPage + 1)
                }
                IconButton {
                    iconName: "last"; tip: qsTr("Last page")
                    enabled: root.controller && root.controller.currentPage < root.controller.pageCount
                    onClicked: root.goToPage(root.controller.pageCount)
                }
                ToolSeparator {}
                IconButton {
                    iconName: "zoomOut"; tip: qsTr("Zoom out")
                    onClicked: { root.scaleType = 3; view.zoom = view.zoom / 1.2 }
                }
                ComboBox {
                    id: zoomBox
                    implicitWidth: 110
                    editable: false
                    model: [qsTr("Fit width"), qsTr("Fit page"), "25%", "50%", "75%", "100%", "150%", "200%", "300%"]
                    displayText: root.scaleType === 0 ? qsTr("Fit width")
                               : root.scaleType === 1 ? qsTr("Fit page")
                               : Math.round(view.zoom / 0.378 * 100) + "%"
                    onActivated: function(index) {
                        if (index === 0) root.fitWidth()
                        else if (index === 1) root.fitPage()
                        else root.setPercent(parseInt(model[index]))
                    }
                }
                IconButton {
                    iconName: "zoomIn"; tip: qsTr("Zoom in")
                    onClicked: { root.scaleType = 3; view.zoom = view.zoom * 1.2 }
                }
                IconButton {
                    iconName: "FitWidth.png"; tip: qsTr("Fit width")
                    onClicked: root.fitWidth()
                }
                IconButton {
                    iconName: "FitPage.png"; tip: qsTr("Fit page")
                    onClicked: root.fitPage()
                }
                IconButton {
                    iconName: "OneToOne.png"; tip: qsTr("One to one")
                    onClicked: root.setPercent(100)
                }
                Item { Layout.fillWidth: true }
                IconButton {
                    iconName: "close"; tip: qsTr("Close")
                    onClicked: root.closeRequested()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            Item {
                SplitView.fillHeight: true
                ReportSceneView {
                    id: view
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.right: vbar.left
                    anchors.bottom: hbar.top
                    scene: root.controller ? root.controller.scene : null
                    interactive: root.controller ? root.controller.editMode : false
                    backgroundColor: root.controller ? root.controller.pageBackgroundColor : "gray"
                    onContentYChanged: {
                        if (root.controller && root.controller.pageCount > 0) {
                            var p = view.mapToScene(width / 2, height / 3)
                            root.controller.currentPage = root.controller.pageAt(p.x, p.y)
                        }
                    }
                    onWidthChanged: if (root.scaleType <= 1) Qt.callLater(root.applyScale)
                    onPopupMenuRequested: function(menu, x, y) { popup.show(menu, x, y) }
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
                BusyIndicator {
                    anchors.centerIn: parent
                    running: root.controller && root.controller.busy
                }
            }

            ScrollView {
                visible: root.controller && root.controller.errorMessages.length > 0
                SplitView.preferredHeight: 100
                TextArea {
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    color: "darkred"
                    text: root.controller ? root.controller.errorMessages.join("\n") : ""
                }
            }
        }

        ToolBar {
            Layout.fillWidth: true
            visible: root.statusBarVisible
            RowLayout {
                anchors.fill: parent
                Label {
                    Layout.leftMargin: 8
                    text: root.controller && root.controller.pageCount > 0
                          ? qsTr("Page: %1 of %2").arg(root.controller.currentPage).arg(root.controller.pageCount)
                          : qsTr("No pages")
                }
                Item { Layout.fillWidth: true }
                Label {
                    Layout.rightMargin: 8
                    text: Math.round(view.zoom / 0.378 * 100) + "%"
                }
            }
        }
    }

    PopupMenuHost { id: popup; parent: view }

    FileDialog {
        id: pdfDialog
        title: qsTr("PDF file name")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF files (*.pdf)")]
        defaultSuffix: "pdf"
        onAccepted: root.controller.exportToPdf(selectedFile)
    }
    FileDialog {
        id: saveDialog
        title: qsTr("Report file name")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Prepared pages (*.lrpx)"), qsTr("All files (*)")]
        defaultSuffix: "lrpx"
        onAccepted: root.controller.savePages(selectedFile)
    }
    FileDialog {
        id: openDialog
        title: qsTr("Open prepared pages")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Prepared pages (*.lrpx)"), qsTr("All files (*)")]
        onAccepted: root.controller.loadPages(selectedFile)
    }
    FileDialog {
        id: exportDialog
        property string exporterName
        title: qsTr("%1 file name").arg(exporterName)
        fileMode: FileDialog.SaveFile
        onAccepted: root.controller.exportTo(exporterName, selectedFile)
    }
    MessageDialog {
        id: errorDialog
        title: qsTr("Error")
        buttons: MessageDialog.Ok
    }
}
