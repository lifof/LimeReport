import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One row of the property inspector. The editor depends on the "editor"
// role of ReportPropertyModel.
Item {
    id: row
    required property int index
    required property string name
    required property var value
    required property string valueText
    required property string editor
    required property var options
    required property bool readOnly

    property var inspector
    property real nameWidth: width * 0.42

    implicitHeight: 30

    function commit(v) { inspector.model.setValue(row.index, v) }

    Rectangle {
        anchors.fill: parent
        color: row.index % 2 ? palette.alternateBase : palette.base
    }

    Label {
        id: nameLabel
        x: 6
        width: row.nameWidth - 8
        anchors.verticalCenter: parent.verticalCenter
        text: row.name
        elide: Text.ElideRight
        font.bold: row.name === "objectName"
    }

    Loader {
        id: loader
        x: row.nameWidth
        width: row.width - row.nameWidth - 2
        anchors.verticalCenter: parent.verticalCenter
        sourceComponent: {
            switch (row.editor) {
            case "string": return stringEditor
            case "text": return textEditor
            case "int": return intEditor
            case "real": return realEditor
            case "bool": return boolEditor
            case "enum": return enumEditor
            case "datasource": return datasourceEditor
            case "flags": return flagsEditor
            case "color": return colorEditor
            case "font": return fontEditor
            case "rect": return rectEditor
            case "image": return imageEditor
            default: return labelEditor
            }
        }
    }

    Component {
        id: labelEditor
        Label { text: row.valueText; elide: Text.ElideRight }
    }

    Component {
        id: stringEditor
        TextField {
            text: row.value !== undefined ? row.value : ""
            selectByMouse: true
            onEditingFinished: if (text !== row.value) row.commit(text)
        }
    }

    Component {
        id: textEditor
        RowLayout {
            spacing: 2
            TextField {
                Layout.fillWidth: true
                text: row.value !== undefined ? row.value : ""
                selectByMouse: true
                onEditingFinished: if (text !== row.value) row.commit(text)
            }
            Button {
                text: "…"
                implicitWidth: 28
                onClicked: row.inspector.editText(row.index, row.name, row.value)
            }
        }
    }

    Component {
        id: intEditor
        SpinBox {
            from: -1000000
            to: 1000000
            editable: true
            value: row.value !== undefined ? row.value : 0
            onValueModified: row.commit(value)
        }
    }

    Component {
        id: realEditor
        TextField {
            text: row.value !== undefined ? Number(row.value).toLocaleString(Qt.locale("C"), 'f', 2) : "0"
            validator: DoubleValidator { locale: "C" }
            selectByMouse: true
            onEditingFinished: row.commit(parseFloat(text))
        }
    }

    Component {
        id: boolEditor
        CheckBox {
            checked: row.value === true
            onToggled: row.commit(checked)
        }
    }

    Component {
        id: enumEditor
        ComboBox {
            model: row.options
            textRole: "text"
            valueRole: "value"
            currentIndex: {
                if (!row.options) return -1
                for (var i = 0; i < row.options.length; ++i)
                    if (row.options[i].value === row.value) return i
                return -1
            }
            onActivated: function(index) { row.commit(row.options[index].value) }
        }
    }

    Component {
        id: datasourceEditor
        ComboBox {
            model: row.options
            editable: true
            currentIndex: row.options ? row.options.indexOf(row.value) : -1
            onActivated: function(index) { row.commit(row.options[index]) }
            onAccepted: row.commit(editText)
        }
    }

    Component {
        id: flagsEditor
        Button {
            text: row.valueText.length > 0 ? row.valueText : qsTr("(none)")
            contentItem: Label { text: parent.text; elide: Text.ElideRight; horizontalAlignment: Text.AlignLeft }
            onClicked: flagsMenu.popup()
            Menu {
                id: flagsMenu
                Repeater {
                    model: row.options
                    MenuItem {
                        required property var modelData
                        text: modelData.text
                        checkable: true
                        checked: (row.value & modelData.value) === modelData.value
                        onTriggered: row.inspector.model.setFlag(row.index, modelData.value, checked)
                    }
                }
            }
        }
    }

    Component {
        id: colorEditor
        RowLayout {
            spacing: 4
            Rectangle {
                implicitWidth: 20
                implicitHeight: 20
                border.color: palette.dark
                color: row.value !== undefined ? row.value : "transparent"
            }
            Button {
                Layout.fillWidth: true
                text: row.valueText
                onClicked: row.inspector.editColor(row.index, row.value)
            }
        }
    }

    Component {
        id: fontEditor
        Button {
            text: row.valueText
            contentItem: Label { text: parent.text; elide: Text.ElideRight; horizontalAlignment: Text.AlignLeft }
            onClicked: row.inspector.editFont(row.index, row.value)
        }
    }

    Component {
        id: rectEditor
        Button {
            text: row.valueText
            contentItem: Label { text: parent.text; elide: Text.ElideRight; horizontalAlignment: Text.AlignLeft }
            onClicked: row.inspector.editRect(row.index, row.value, this)
        }
    }

    Component {
        id: imageEditor
        RowLayout {
            spacing: 2
            Label { Layout.fillWidth: true; text: row.valueText; elide: Text.ElideRight }
            Button { text: qsTr("Load…"); onClicked: row.inspector.loadImage(row.index) }
            Button { text: qsTr("Clear"); onClicked: row.inspector.model.clearValue(row.index) }
        }
    }
}
