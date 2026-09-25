import QtQuick
import QtQuick.Controls

// Tree of the items of the current report page.
ListView {
    id: tree
    property var designer: null
    clip: true
    model: designer ? designer.objectTreeModel : null
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    delegate: TreeRow {
        width: tree.width
        treeModel: tree.model
        iconName: kind.indexOf("Band") >= 0 || kind.indexOf("Header") >= 0 || kind.indexOf("Footer") >= 0
                  ? "addBand" : (kind === "PageItemDesignIntf" ? "newReport" : "object")
        onActivated: if (tree.designer && object) tree.designer.selectObject(object, false)
    }
}
