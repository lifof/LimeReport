import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import LimeReport

/*
 * The LimeReport report designer.
 *
 *   ReportEngine { id: engine }
 *   ReportDesigner { anchors.fill: parent; engine: engine }
 */
Item {
    id: root

    property QtObject engine: null
    readonly property ReportDesignerController designer: controller
    readonly property bool modified: controller.modified
    readonly property string title: controller.reportName + (controller.modified ? " *" : "")
    property bool showScript: false

    // Runs action() once unsaved changes have been saved or discarded.
    function confirmDiscard(action) {
        if (!controller.modified) { action(); return }
        saveChangesDialog.pendingAction = action
        saveChangesDialog.open()
    }
    function newReport() { confirmDiscard(function() { controller.newReport() }) }
    function openReport() { confirmDiscard(function() { openDialog.currentFolder = controller.reportFolder(); openDialog.open() }) }
    function saveReport(then) {
        if (controller.saveReport()) { if (then) then(); return }
        saveAsDialog.then = then || null
        saveAsDialog.currentFolder = controller.reportFolder()
        saveAsDialog.open()
    }
    function saveReportAs() {
        saveAsDialog.then = null
        saveAsDialog.currentFolder = controller.reportFolder()
        saveAsDialog.open()
    }

    ReportDesignerController {
        id: controller
        engine: root.engine
        onEditItemRequested: function(item, kind) {
            if (kind === "text") {
                contentDialog.item = item
                contentDialog.text = controller.itemContent(item)
                contentDialog.open()
            } else if (kind === "image" || kind === "svg") {
                imageDialog.item = item
                imageDialog.open()
            } else {
                rightTabs.currentIndex = 0
            }
        }
        onMessage: function(severity, title, text) {
            messageDialog.title = title
            messageDialog.text = text
            messageDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            Flow {
                width: parent.width
                spacing: 0
                IconButton { iconName: "newReport"; tip: qsTr("New report (Ctrl+N)"); onClicked: root.newReport() }
                IconButton { iconName: "folder"; tip: qsTr("Open report (Ctrl+O)"); onClicked: root.openReport() }
                IconButton { iconName: "save"; tip: qsTr("Save report (Ctrl+S)"); onClicked: root.saveReport() }
                IconButton { iconName: "saveas"; tip: qsTr("Save report as"); onClicked: root.saveReportAs() }
                ToolSeparator {}
                IconButton { iconName: "undo"; tip: qsTr("Undo (Ctrl+Z)"); enabled: controller.canUndo; onClicked: controller.undo() }
                IconButton { iconName: "redo"; tip: qsTr("Redo (Ctrl+Y)"); enabled: controller.canRedo; onClicked: controller.redo() }
                ToolSeparator {}
                IconButton { iconName: "copy"; tip: qsTr("Copy (Ctrl+C)"); enabled: controller.selectionCount > 0; onClicked: controller.copy() }
                IconButton { iconName: "cut"; tip: qsTr("Cut (Ctrl+X)"); enabled: controller.selectionCount > 0; onClicked: controller.cut() }
                IconButton { iconName: "paste"; tip: qsTr("Paste (Ctrl+V)"); onClicked: controller.paste() }
                IconButton { iconName: "delete"; tip: qsTr("Delete (Del)"); enabled: controller.selectionCount > 0; onClicked: controller.deleteSelected() }
                ToolSeparator {}
                IconButton { iconName: "render"; tip: qsTr("Render report (Ctrl+P)"); onClicked: controller.preview() }
                IconButton { iconName: "pdf"; tip: qsTr("Export to PDF"); onClicked: { pdfDialog.currentFolder = controller.reportFolder(); pdfDialog.open() } }
                IconButton { iconName: "print"; tip: qsTr("Print"); onClicked: controller.print() }
                ToolSeparator {}
                IconButton { iconName: "addPage"; tip: qsTr("Add page"); onClicked: controller.addPage() }
                IconButton {
                    iconName: "deletePage"; tip: qsTr("Delete page")
                    enabled: controller.pageNames.length > 1 && !root.showScript
                    onClicked: controller.deletePage(controller.currentPageIndex)
                }
                ToolSeparator {}
                IconButton { iconName: "zoomOut"; tip: qsTr("Zoom out"); onClicked: canvas.zoomOut() }
                IconButton { iconName: "zoomIn"; tip: qsTr("Zoom in"); onClicked: canvas.zoomIn() }
                IconButton { iconName: "FitWidth.png"; tip: qsTr("Fit width"); onClicked: canvas.fitWidth() }
                ToolSeparator {}
                IconButton {
                    iconName: "grid"; tip: qsTr("Use grid"); checkable: true
                    checked: controller.useGrid; onToggled: controller.useGrid = checked
                }
                IconButton {
                    iconName: "magnet"; tip: qsTr("Use magnet"); checkable: true
                    checked: controller.magneticMovement; onToggled: controller.magneticMovement = checked
                }
                IconButton {
                    iconName: "editlayout"; tip: qsTr("Edit layouts mode"); checkable: true
                    checked: controller.layoutEditMode; onToggled: controller.layoutEditMode = checked
                }
            }
        }

        ToolBar {
            Layout.fillWidth: true
            Flow {
                width: parent.width
                spacing: 0
                IconButton {
                    iconName: "edit"; tip: qsTr("Select mode (Esc)"); checkable: true
                    checked: controller.insertItemType.length === 0
                    onClicked: controller.cancelInsert()
                }
                Repeater {
                    model: controller.itemTypes
                    ToolButton {
                        required property var modelData
                        text: modelData.name
                        display: modelData.icon.length > 0 ? AbstractButton.IconOnly : AbstractButton.TextOnly
                        icon.source: modelData.icon
                        icon.color: "transparent"
                        focusPolicy: Qt.NoFocus
                        checkable: true
                        checked: controller.insertItemType === modelData.type
                        onClicked: controller.startInsert(modelData.type)
                        ToolTip.visible: hovered
                        ToolTip.text: modelData.name
                        ToolTip.delay: 600
                    }
                }
                IconButton {
                    iconName: "addBand"; tip: qsTr("Add band")
                    onClicked: bandsMenu.popup()
                    Menu {
                        id: bandsMenu
                        Repeater {
                            model: controller.bandTypes
                            MenuItem {
                                required property var modelData
                                text: modelData.name
                                enabled: modelData.enabled
                                onTriggered: controller.addBand(modelData.type)
                            }
                        }
                    }
                }
                ToolSeparator {}
                IconButton { iconName: "bringToTop"; tip: qsTr("Bring to front"); onClicked: controller.bringToFront() }
                IconButton { iconName: "sendToBack"; tip: qsTr("Send to back"); onClicked: controller.sendToBack() }
                IconButton { iconName: "alignToLeft"; tip: qsTr("Align to left"); onClicked: controller.align("left") }
                IconButton { iconName: "alignToRight"; tip: qsTr("Align to right"); onClicked: controller.align("right") }
                IconButton { iconName: "alignToVCenter"; tip: qsTr("Align to vertical center"); onClicked: controller.align("vcenter") }
                IconButton { iconName: "alignToTop"; tip: qsTr("Align to top"); onClicked: controller.align("top") }
                IconButton { iconName: "alignToBottom"; tip: qsTr("Align to bottom"); onClicked: controller.align("bottom") }
                IconButton { iconName: "alignToHCenter"; tip: qsTr("Align to horizontal center"); onClicked: controller.align("hcenter") }
                IconButton { iconName: "sameHeight"; tip: qsTr("Set same height"); onClicked: controller.sameHeight() }
                IconButton { iconName: "sameWidth"; tip: qsTr("Set same width"); onClicked: controller.sameWidth() }
                IconButton { iconName: "hlayout"; tip: qsTr("Create horizontal layout"); enabled: controller.selectionCount > 1; onClicked: controller.addHLayout() }
                IconButton { iconName: "vlayout"; tip: qsTr("Create vertical layout"); enabled: controller.selectionCount > 1; onClicked: controller.addVLayout() }
                IconButton { iconName: "lock"; tip: qsTr("Lock selected items"); onClicked: controller.lockSelected() }
                IconButton { iconName: "unlock.png"; tip: qsTr("Unlock selected items"); onClicked: controller.unlockSelected() }
                ToolSeparator {}
                IconButton { iconName: "textBold"; tip: qsTr("Bold"); checkable: true; onToggled: controller.setFontStyle("bold", checked) }
                IconButton { iconName: "textItalic"; tip: qsTr("Italic"); checkable: true; onToggled: controller.setFontStyle("italic", checked) }
                IconButton { iconName: "textUnderline"; tip: qsTr("Underline"); checkable: true; onToggled: controller.setFontStyle("underline", checked) }
                IconButton { iconName: "textAlignHLeft"; tip: qsTr("Align text left"); onClicked: controller.setTextAlignment(true, Qt.AlignLeft) }
                IconButton { iconName: "textAlignHCenter"; tip: qsTr("Center text"); onClicked: controller.setTextAlignment(true, Qt.AlignHCenter) }
                IconButton { iconName: "textAlignHRight"; tip: qsTr("Align text right"); onClicked: controller.setTextAlignment(true, Qt.AlignRight) }
                IconButton { iconName: "textAlignHJustify"; tip: qsTr("Justify text"); onClicked: controller.setTextAlignment(true, Qt.AlignJustify) }
                IconButton { iconName: "textAlignVTop"; tip: qsTr("Align text top"); onClicked: controller.setTextAlignment(false, Qt.AlignTop) }
                IconButton { iconName: "textAlignVCenter"; tip: qsTr("Align text vertical center"); onClicked: controller.setTextAlignment(false, Qt.AlignVCenter) }
                IconButton { iconName: "textAlignVBottom"; tip: qsTr("Align text bottom"); onClicked: controller.setTextAlignment(false, Qt.AlignBottom) }
                ToolSeparator {}
                IconButton { iconName: "noLines"; tip: qsTr("No borders"); onClicked: controller.setBorders(0) }
                IconButton { iconName: "allLines"; tip: qsTr("All borders"); onClicked: controller.setBorders(15) }
                IconButton { iconName: "topLine"; tip: qsTr("Top border"); onClicked: controller.setBorders(1) }
                IconButton { iconName: "bottomLine"; tip: qsTr("Bottom border"); onClicked: controller.setBorders(2) }
                IconButton { iconName: "leftLine"; tip: qsTr("Left border"); onClicked: controller.setBorders(4) }
                IconButton { iconName: "rightLine"; tip: qsTr("Right border"); onClicked: controller.setBorders(8) }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ColumnLayout {
                SplitView.preferredWidth: 260
                SplitView.minimumWidth: 150
                spacing: 0
                TabBar {
                    id: leftTabs
                    Layout.fillWidth: true
                    TabButton { text: qsTr("Objects") }
                    TabButton { text: qsTr("Data") }
                }
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: leftTabs.currentIndex
                    ObjectTree { designer: controller }
                    DataBrowser { designer: controller }
                }
            }

            ColumnLayout {
                SplitView.fillWidth: true
                spacing: 0
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.showScript ? 1 : 0
                    DesignerCanvasView {
                        id: canvas
                        designer: controller
                        scene: controller.scene
                    }
                    ScrollView {
                        TextArea {
                            id: scriptArea
                            font.family: "monospace"
                            wrapMode: TextEdit.NoWrap
                            selectByMouse: true
                            placeholderText: qsTr("Report init script (JavaScript)")
                            text: controller.script
                            onTextChanged: if (activeFocus) controller.script = text
                        }
                    }
                }
                TabBar {
                    id: pageTabs
                    Layout.fillWidth: true
                    position: TabBar.Footer
                    Repeater {
                        model: controller.pageNames
                        TabButton {
                            required property string modelData
                            required property int index
                            text: modelData
                            width: implicitWidth
                            checked: !root.showScript && controller.currentPageIndex === index
                            onClicked: { root.showScript = false; controller.currentPageIndex = index }
                        }
                    }
                    TabButton {
                        text: qsTr("Script")
                        width: implicitWidth
                        checked: root.showScript
                        onClicked: root.showScript = true
                    }
                }
            }

            ColumnLayout {
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: 200
                spacing: 0
                TabBar {
                    id: rightTabs
                    Layout.fillWidth: true
                    TabButton { text: qsTr("Properties") }
                }
                PropertyInspector {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: controller.propertyModel
                }
            }
        }

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Label {
                    Layout.leftMargin: 8
                    text: controller.reportFileName.length > 0 ? controller.reportFileName : controller.reportName
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                Label {
                    visible: controller.insertItemType.length > 0
                    text: qsTr("Click on the page to insert %1, Esc to cancel").arg(controller.insertItemType)
                }
                Label {
                    Layout.rightMargin: 8
                    text: Math.round(canvas.zoom / 0.378 * 100) + "%"
                }
            }
        }
    }

    // --- shortcuts -------------------------------------------------------
    Shortcut { sequences: [StandardKey.New]; onActivated: root.newReport() }
    Shortcut { sequences: [StandardKey.Open]; onActivated: root.openReport() }
    Shortcut { sequences: [StandardKey.Save]; onActivated: root.saveReport() }
    Shortcut { sequence: "Ctrl+P"; onActivated: controller.preview() }
    Shortcut { sequences: [StandardKey.Undo]; enabled: canvas.view.activeFocus; onActivated: controller.undo() }
    Shortcut { sequences: [StandardKey.Redo]; enabled: canvas.view.activeFocus; onActivated: controller.redo() }
    Shortcut { sequences: [StandardKey.Copy]; enabled: canvas.view.activeFocus; onActivated: controller.copy() }
    Shortcut { sequences: [StandardKey.Cut]; enabled: canvas.view.activeFocus; onActivated: controller.cut() }
    Shortcut { sequences: [StandardKey.Paste]; enabled: canvas.view.activeFocus; onActivated: controller.paste() }
    Shortcut { sequences: [StandardKey.Delete]; enabled: canvas.view.activeFocus; onActivated: controller.deleteSelected() }
    Shortcut { sequence: "Escape"; enabled: controller.insertItemType.length > 0; onActivated: controller.cancelInsert() }
    Shortcut { sequences: [StandardKey.ZoomIn]; onActivated: canvas.zoomIn() }
    Shortcut { sequences: [StandardKey.ZoomOut]; onActivated: canvas.zoomOut() }
    Shortcut { sequence: "Ctrl+L"; enabled: canvas.view.activeFocus; onActivated: controller.lockSelected() }

    // --- dialogs ---------------------------------------------------------
    FileDialog {
        id: openDialog
        title: qsTr("Report file name")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Report files (*.lrxml)"), qsTr("All files (*)")]
        onAccepted: controller.openReport(selectedFile)
    }
    FileDialog {
        id: saveAsDialog
        property var then: null
        title: qsTr("Report file name")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "lrxml"
        nameFilters: [qsTr("Report files (*.lrxml)"), qsTr("All files (*)")]
        onAccepted: {
            if (controller.saveReportAs(selectedFile) && then) then()
            then = null
        }
    }
    FileDialog {
        id: pdfDialog
        title: qsTr("PDF file name")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "pdf"
        nameFilters: [qsTr("PDF files (*.pdf)")]
        onAccepted: controller.exportToPdf(selectedFile)
    }
    FileDialog {
        id: imageDialog
        property var item: null
        title: qsTr("Select image")
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.svg)"), qsTr("All files (*)")]
        onAccepted: {
            controller.selectObject(item, false)
            var model = controller.propertyModel
            var row = model.rowOf("image")
            if (row >= 0) model.loadImage(row, selectedFile)
        }
    }
    ContentEditorDialog {
        id: contentDialog
        property var item: null
        title: qsTr("Edit text")
        onAccepted: controller.setItemContent(item, text)
    }
    Dialog {
        id: saveChangesDialog
        property var pendingAction: null
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Report has been modified")
        Label { text: qsTr("Do you want to save the report?") }
        footer: DialogButtonBox {
            Button { text: qsTr("Save"); DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
            Button { text: qsTr("Discard"); DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole }
            Button { text: qsTr("Cancel"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            onAccepted: {
                saveChangesDialog.close()
                root.saveReport(saveChangesDialog.pendingAction)
            }
            onClicked: function(button) {
                if (button.DialogButtonBox.buttonRole === DialogButtonBox.DestructiveRole) {
                    saveChangesDialog.close()
                    var action = saveChangesDialog.pendingAction
                    saveChangesDialog.pendingAction = null
                    if (action) action()
                }
            }
            onRejected: saveChangesDialog.close()
        }
    }
    MessageDialog {
        id: messageDialog
        buttons: MessageDialog.Ok
    }
}
