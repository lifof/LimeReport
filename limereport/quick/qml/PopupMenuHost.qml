import QtQuick
import QtQuick.Controls

// Shows a LimeReport::PopupMenu (a context menu built by report items).
Menu {
    id: menu
    property var popupMenu: null
    property var createdItems: []

    function clearEntries() {
        for (var i = 0; i < createdItems.length; ++i) {
            menu.removeItem(createdItems[i])
            createdItems[i].destroy()
        }
        createdItems = []
    }

    function show(pm, x, y) {
        clearEntries()
        if (popupMenu) popupMenu.dispose()
        popupMenu = pm
        if (!pm) return
        var entries = pm.items()
        var created = []
        var lastWasSeparator = true
        for (var i = 0; i < entries.length; ++i) {
            var e = entries[i]
            if (!e.visible) continue
            var item
            if (e.separator) {
                if (lastWasSeparator) continue
                item = separatorComponent.createObject(null)
                lastWasSeparator = true
            } else {
                item = itemComponent.createObject(null, {
                    "text": e.text,
                    "enabled": e.enabled,
                    "checkable": e.checkable,
                    "checked": e.checked,
                    "actionIndex": i
                })
                lastWasSeparator = false
            }
            menu.addItem(item)
            created.push(item)
        }
        createdItems = created
        popup(x, y)
    }

    onClosed: {
        var pm = popupMenu
        popupMenu = null
        if (pm) Qt.callLater(function() { pm.dispose() })
    }

    Component {
        id: itemComponent
        MenuItem {
            property int actionIndex: -1
            onTriggered: if (menu.popupMenu) menu.popupMenu.trigger(actionIndex)
        }
    }
    Component {
        id: separatorComponent
        MenuSeparator {}
    }
}
