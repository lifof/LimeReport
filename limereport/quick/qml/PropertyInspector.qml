import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// Object inspector: edits the properties of the selected report items.
Pane {
    id: inspectorRoot
    property var model: null
    property string filter: ""
    padding: 0

    function editText(row, name, value) {
        textDialog.row = row
        textDialog.title = name
        textDialog.text = value !== undefined ? value : ""
        textDialog.open()
    }
    function editColor(row, value) {
        colorDialog.row = row
        colorDialog.selectedColor = value
        colorDialog.open()
    }
    function editFont(row, value) {
        fontDialog.row = row
        fontDialog.selectedFont = Qt.font({
            family: value.family, pointSize: value.pointSize, bold: value.bold,
            italic: value.italic, underline: value.underline, strikeout: value.strikeout
        })
        fontDialog.open()
    }
    function editRect(row, value, anchorItem) {
        rectPopup.row = row
        rectPopup.parent = anchorItem
        rectX.text = value.x.toFixed(2)
        rectY.text = value.y.toFixed(2)
        rectW.text = value.width.toFixed(2)
        rectH.text = value.height.toFixed(2)
        rectPopup.open()
    }
    function loadImage(row) {
        imageDialog.row = row
        imageDialog.open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        Label {
            Layout.fillWidth: true
            Layout.margins: 6
            text: inspectorRoot.model && inspectorRoot.model.objectCount > 0
                  ? inspectorRoot.model.objectName + "  (" + inspectorRoot.model.objectType + ")"
                  : qsTr("No selection")
            font.bold: true
            elide: Text.ElideRight
        }
        TextField {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            placeholderText: qsTr("Filter properties")
            onTextChanged: inspectorRoot.filter = text.toLowerCase()
        }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: inspectorRoot.model
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            section.property: "section"
            section.delegate: Rectangle {
                required property string section
                width: list.width
                height: 22
                color: palette.button
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 6
                    text: parent.section
                    font.italic: true
                }
            }
            delegate: PropertyEditor {
                width: list.width
                inspector: inspectorRoot
                visible: inspectorRoot.filter.length === 0 || name.toLowerCase().indexOf(inspectorRoot.filter) >= 0
                height: visible ? implicitHeight : 0
            }
        }
    }

    ContentEditorDialog {
        id: textDialog
        property int row: -1
        onAccepted: inspectorRoot.model.setValue(row, text)
    }
    ColorDialog {
        id: colorDialog
        property int row: -1
        options: ColorDialog.ShowAlphaChannel
        onAccepted: inspectorRoot.model.setValue(row, selectedColor)
    }
    FontDialog {
        id: fontDialog
        property int row: -1
        onAccepted: inspectorRoot.model.setFontValue(row, selectedFont.family, selectedFont.pointSize,
                                                 selectedFont.bold, selectedFont.italic,
                                                 selectedFont.underline, selectedFont.strikeout)
    }
    FileDialog {
        id: imageDialog
        property int row: -1
        title: qsTr("Select image")
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.svg)"), qsTr("All files (*)")]
        onAccepted: inspectorRoot.model.loadImage(row, selectedFile)
    }
    Popup {
        id: rectPopup
        property int row: -1
        y: parent ? parent.height : 0
        modal: true
        focus: true
        contentItem: GridLayout {
            columns: 2
            Label { text: qsTr("Left") }   TextField { id: rectX; validator: DoubleValidator { locale: "C" } }
            Label { text: qsTr("Top") }    TextField { id: rectY; validator: DoubleValidator { locale: "C" } }
            Label { text: qsTr("Width") }  TextField { id: rectW; validator: DoubleValidator { locale: "C" } }
            Label { text: qsTr("Height") } TextField { id: rectH; validator: DoubleValidator { locale: "C" } }
            Button {
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignRight
                text: qsTr("Apply")
                onClicked: {
                    inspectorRoot.model.setRectValue(rectPopup.row, parseFloat(rectX.text), parseFloat(rectY.text),
                                                 parseFloat(rectW.text), parseFloat(rectH.text))
                    rectPopup.close()
                }
            }
        }
    }
}
