import QtQuick
import QtQuick.Controls

// Tool button showing a LimeReport resource icon with a tooltip.
ToolButton {
    id: control
    property string iconName
    property string tip: text

    display: iconName.length > 0 ? AbstractButton.IconOnly : AbstractButton.TextOnly
    icon.source: iconName.length > 0 ? "qrc:/report/images/" + iconName : ""
    icon.color: "transparent"
    icon.width: 20
    icon.height: 20
    focusPolicy: Qt.NoFocus
    hoverEnabled: true

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 600
}
