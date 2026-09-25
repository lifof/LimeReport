import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LimeReport
import DemoR1

// Demo: pick a report, preview it embedded, or open the designer / preview windows.
ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: qsTr("LimeReport demo")

    property string currentReport: ""

    function renderSelected() {
        if (currentReport.length === 0) return
        DemoBackend.setVariable(varName.text, varValue.text)
        if (DemoBackend.load(currentReport)) previewController.render()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Button {
                text: qsTr("Design report")
                onClicked: {
                    DemoBackend.setVariable(varName.text, varValue.text)
                    if (window.currentReport.length > 0) DemoBackend.load(window.currentReport)
                    DemoBackend.designReport()
                }
            }
            Button {
                text: qsTr("Preview in window")
                enabled: window.currentReport.length > 0
                onClicked: {
                    DemoBackend.setVariable(varName.text, varValue.text)
                    if (DemoBackend.load(window.currentReport)) DemoBackend.previewReport()
                }
            }
            ToolSeparator {}
            Label { text: qsTr("Variable") }
            TextField { id: varName; placeholderText: qsTr("name") }
            TextField { id: varValue; placeholderText: qsTr("value") }
            Item { Layout.fillWidth: true }
            Label {
                id: progress
                Layout.rightMargin: 8
                Connections {
                    target: DemoBackend.engine
                    function onRenderStarted() { progress.text = qsTr("Rendering…") }
                    function onRenderPageFinished(count) { progress.text = qsTr("%1 page(s) rendered").arg(count) }
                    function onRenderFinished() { progress.text = "" }
                }
            }
        }
    }

    SplitView {
        anchors.fill: parent
        ListView {
            id: reportList
            SplitView.preferredWidth: 300
            clip: true
            model: DemoBackend.reports
            delegate: ItemDelegate {
                required property string modelData
                required property int index
                width: reportList.width
                text: modelData
                highlighted: ListView.isCurrentItem
                onClicked: {
                    reportList.currentIndex = index
                    window.currentReport = modelData
                    window.renderSelected()
                }
            }
        }
        ReportPreview {
            SplitView.fillWidth: true
            controller: ReportPreviewController {
                id: previewController
                engine: DemoBackend.engine
            }
        }
    }
}
