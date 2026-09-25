/***************************************************************************
 *   This file is part of the Lime Report project                          *
 *   Copyright (C) 2021 by Alexander Arin                                  *
 *   arin_a@bk.ru                                                          *
 *                                                                         *
 **                   GNU General Public License Usage                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 **                  GNU Lesser General Public License                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library.                                      *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ****************************************************************************/
#ifndef LRGRAPHICSSCENE_H
#define LRGRAPHICSSCENE_H

/*
 * A lightweight, QtGui-only retained scene graph.
 *
 * LimeReport used to be built on QGraphicsScene / QGraphicsItem, which live
 * in the QtWidgets module. This file provides the subset of that API the
 * report engine relies on (hierarchy, stacking, selection, hit testing,
 * hover/mouse/key/drop dispatch and QPainter rendering) without any widget
 * dependency, so the engine can be hosted by Qt Quick or rendered headless.
 */

#include <QBrush>
#include <QCursor>
#include <QIcon>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QPointer>
#include <QPolygonF>
#include <QRectF>
#include <QTransform>
#include <QVariant>

class QPainter;
class QMimeData;
class QKeyEvent;
class QAction;

namespace LimeReport {

class GraphicsItem;
class GraphicsScene;

class StyleOptionGraphicsItem {
public:
    enum StateFlag {
        State_None = 0,
        State_Selected = 0x1,
        State_MouseOver = 0x2
    };
    QRect rect;
    QRectF exposedRect;
    int state = State_None;
};

class GraphicsSceneEvent {
public:
    virtual ~GraphicsSceneEvent() { }
    void accept() { m_accepted = true; }
    void ignore() { m_accepted = false; }
    bool isAccepted() const { return m_accepted; }
    void setAccepted(bool value) { m_accepted = value; }
    Qt::KeyboardModifiers modifiers() const { return m_modifiers; }
    void setModifiers(Qt::KeyboardModifiers value) { m_modifiers = value; }

private:
    bool m_accepted = true;
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;
};

class GraphicsSceneMouseEvent: public GraphicsSceneEvent {
public:
    QPointF pos() const { return m_pos; }
    void setPos(const QPointF& value) { m_pos = value; }
    QPointF scenePos() const { return m_scenePos; }
    void setScenePos(const QPointF& value) { m_scenePos = value; }
    QPointF screenPos() const { return m_screenPos; }
    void setScreenPos(const QPointF& value) { m_screenPos = value; }
    QPointF lastPos() const { return m_lastPos; }
    void setLastPos(const QPointF& value) { m_lastPos = value; }
    QPointF lastScenePos() const { return m_lastScenePos; }
    void setLastScenePos(const QPointF& value) { m_lastScenePos = value; }
    QPointF buttonDownPos(Qt::MouseButton button) const { return m_buttonDownPos.value(button); }
    void setButtonDownPos(Qt::MouseButton button, const QPointF& value)
    {
        m_buttonDownPos[button] = value;
    }
    QPointF buttonDownScenePos(Qt::MouseButton button) const
    {
        return m_buttonDownScenePos.value(button);
    }
    void setButtonDownScenePos(Qt::MouseButton button, const QPointF& value)
    {
        m_buttonDownScenePos[button] = value;
    }
    Qt::MouseButton button() const { return m_button; }
    void setButton(Qt::MouseButton value) { m_button = value; }
    Qt::MouseButtons buttons() const { return m_buttons; }
    void setButtons(Qt::MouseButtons value) { m_buttons = value; }

private:
    QPointF m_pos;
    QPointF m_scenePos;
    QPointF m_screenPos;
    QPointF m_lastPos;
    QPointF m_lastScenePos;
    QMap<Qt::MouseButton, QPointF> m_buttonDownPos;
    QMap<Qt::MouseButton, QPointF> m_buttonDownScenePos;
    Qt::MouseButton m_button = Qt::NoButton;
    Qt::MouseButtons m_buttons = Qt::NoButton;
};

class GraphicsSceneHoverEvent: public GraphicsSceneEvent {
public:
    QPointF pos() const { return m_pos; }
    void setPos(const QPointF& value) { m_pos = value; }
    QPointF scenePos() const { return m_scenePos; }
    void setScenePos(const QPointF& value) { m_scenePos = value; }
    QPointF screenPos() const { return m_screenPos; }
    void setScreenPos(const QPointF& value) { m_screenPos = value; }

private:
    QPointF m_pos;
    QPointF m_scenePos;
    QPointF m_screenPos;
};

class GraphicsSceneContextMenuEvent: public GraphicsSceneEvent {
public:
    QPointF pos() const { return m_pos; }
    void setPos(const QPointF& value) { m_pos = value; }
    QPointF scenePos() const { return m_scenePos; }
    void setScenePos(const QPointF& value) { m_scenePos = value; }
    QPoint screenPos() const { return m_screenPos; }
    void setScreenPos(const QPoint& value) { m_screenPos = value; }

private:
    QPointF m_pos;
    QPointF m_scenePos;
    QPoint m_screenPos;
};

class GraphicsSceneDragDropEvent: public GraphicsSceneEvent {
public:
    QPointF pos() const { return m_pos; }
    void setPos(const QPointF& value) { m_pos = value; }
    QPointF scenePos() const { return m_scenePos; }
    void setScenePos(const QPointF& value) { m_scenePos = value; }
    const QMimeData* mimeData() const { return m_mimeData; }
    void setMimeData(const QMimeData* value) { m_mimeData = value; }
    Qt::DropAction dropAction() const { return m_dropAction; }
    void setDropAction(Qt::DropAction value) { m_dropAction = value; }
    Qt::DropAction proposedAction() const { return m_proposedAction; }
    void setProposedAction(Qt::DropAction value) { m_proposedAction = value; }
    void acceptProposedAction()
    {
        m_dropAction = m_proposedAction;
        accept();
    }

private:
    QPointF m_pos;
    QPointF m_scenePos;
    const QMimeData* m_mimeData = nullptr;
    Qt::DropAction m_dropAction = Qt::CopyAction;
    Qt::DropAction m_proposedAction = Qt::CopyAction;
};

/*
 * A context menu description. Items build it synchronously, the hosting view
 * (for example the QML designer canvas) shows it and calls trigger().
 */
class PopupMenu: public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> actions READ actionObjects NOTIFY actionsChanged)
public:
    explicit PopupMenu(QObject* parent = nullptr): QObject(parent) { }
    QAction* addAction(const QString& text);
    QAction* addAction(const QIcon& icon, const QString& text);
    QAction* addSeparator();
    void insertAction(QAction* before, QAction* action);
    QAction* insertSeparator(QAction* before);
    QList<QAction*> actions() const { return m_actions; }
    QList<QObject*> actionObjects() const;
    Q_INVOKABLE void trigger(int index);
    // Plain description of the entries for QML: text, enabled, visible,
    // checkable, checked, separator and shortcut.
    Q_INVOKABLE QVariantList items() const;
    // Releases the menu once the view has closed it.
    Q_INVOKABLE void dispose() { deleteLater(); }
signals:
    void actionsChanged();
    void triggered(QAction* action);

private:
    QList<QAction*> m_actions;
};

class GraphicsItem {
public:
    enum GraphicsItemFlag {
        ItemIsMovable = 0x1,
        ItemIsSelectable = 0x2,
        ItemIsFocusable = 0x4,
        ItemClipsToShape = 0x8,
        ItemClipsChildrenToShape = 0x10,
        ItemStacksBehindParent = 0x100,
        ItemSendsGeometryChanges = 0x800,
        ItemNegativeZStacksBehindParent = 0x2000
    };
    Q_DECLARE_FLAGS(GraphicsItemFlags, GraphicsItemFlag)

    enum GraphicsItemChange {
        ItemPositionChange,
        ItemVisibleChange = 2,
        ItemEnabledChange,
        ItemSelectedChange,
        ItemParentChange,
        ItemChildAddedChange,
        ItemChildRemovedChange,
        ItemTransformChange,
        ItemPositionHasChanged,
        ItemTransformHasChanged,
        ItemSceneChange,
        ItemVisibleHasChanged,
        ItemEnabledHasChanged,
        ItemSelectedHasChanged,
        ItemParentHasChanged,
        ItemSceneHasChanged,
        ItemCursorChange,
        ItemCursorHasChanged,
        ItemFlagsChange = 20,
        ItemFlagsHaveChanged,
        ItemZValueChange,
        ItemZValueHasChanged,
        ItemScaleChange = 29,
        ItemScaleHasChanged
    };

    enum {
        Type = 1,
        UserType = 65536
    };

    explicit GraphicsItem(GraphicsItem* parent = nullptr);
    virtual ~GraphicsItem();

    virtual QRectF boundingRect() const = 0;
    virtual void paint(QPainter* painter, const StyleOptionGraphicsItem* option) = 0;
    virtual QPainterPath shape() const;
    virtual bool contains(const QPointF& point) const;
    virtual int type() const { return Type; }

    GraphicsScene* scene() const { return m_scene; }
    GraphicsItem* parentItem() const { return m_parent; }
    GraphicsItem* topLevelItem() const;
    void setParentItem(GraphicsItem* parent);
    QList<GraphicsItem*> childItems() const;
    bool isAncestorOf(const GraphicsItem* child) const;

    QPointF pos() const { return m_pos; }
    qreal x() const { return m_pos.x(); }
    qreal y() const { return m_pos.y(); }
    void setPos(const QPointF& pos);
    void setPos(qreal x, qreal y) { setPos(QPointF(x, y)); }
    void setX(qreal x) { setPos(QPointF(x, m_pos.y())); }
    void setY(qreal y) { setPos(QPointF(m_pos.x(), y)); }
    void moveBy(qreal dx, qreal dy) { setPos(m_pos + QPointF(dx, dy)); }
    QPointF scenePos() const;

    qreal scale() const { return m_scale; }
    void setScale(qreal scale);

    qreal zValue() const { return m_z; }
    void setZValue(qreal z);

    bool isVisible() const;
    void setVisible(bool visible);
    void show() { setVisible(true); }
    void hide() { setVisible(false); }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    GraphicsItemFlags flags() const { return m_flags; }
    void setFlag(GraphicsItemFlag flag, bool enabled = true);
    void setFlags(GraphicsItemFlags flags);

    bool acceptHoverEvents() const { return m_acceptHover; }
    void setAcceptHoverEvents(bool enabled) { m_acceptHover = enabled; }
    Qt::MouseButtons acceptedMouseButtons() const { return m_acceptedButtons; }
    void setAcceptedMouseButtons(Qt::MouseButtons buttons) { m_acceptedButtons = buttons; }
    bool acceptDrops() const { return m_acceptDrops; }
    void setAcceptDrops(bool on) { m_acceptDrops = on; }

    QCursor cursor() const { return m_cursor; }
    void setCursor(const QCursor& cursor);
    void unsetCursor();
    bool hasCursor() const { return m_hasCursor; }

    bool hasFocus() const;
    void setFocus();
    void clearFocus();

    QVariant data(int key) const { return m_data.value(key); }
    void setData(int key, const QVariant& value) { m_data.insert(key, value); }

    void update(const QRectF& rect = QRectF());
    void update(qreal x, qreal y, qreal width, qreal height)
    {
        update(QRectF(x, y, width, height));
    }

    QTransform transform() const;
    QTransform sceneTransform() const;
    QRectF sceneBoundingRect() const;

    QPointF mapToScene(const QPointF& point) const;
    QPointF mapToScene(qreal x, qreal y) const { return mapToScene(QPointF(x, y)); }
    QPolygonF mapToScene(const QRectF& rect) const;
    QPainterPath mapToScene(const QPainterPath& path) const;
    QPointF mapFromScene(const QPointF& point) const;
    QPointF mapFromScene(qreal x, qreal y) const { return mapFromScene(QPointF(x, y)); }
    QPolygonF mapFromScene(const QRectF& rect) const;
    QPointF mapToParent(const QPointF& point) const;
    QPointF mapToParent(qreal x, qreal y) const { return mapToParent(QPointF(x, y)); }
    QPointF mapFromParent(const QPointF& point) const;
    QPointF mapToItem(const GraphicsItem* item, const QPointF& point) const;
    QPointF mapFromItem(const GraphicsItem* item, const QPointF& point) const;
    QPointF mapFromItem(const GraphicsItem* item, qreal x, qreal y) const
    {
        return mapFromItem(item, QPointF(x, y));
    }
    QRectF mapRectToScene(const QRectF& rect) const;
    QRectF mapRectFromScene(const QRectF& rect) const;
    QRectF mapRectToParent(const QRectF& rect) const;
    QRectF mapRectFromParent(const QRectF& rect) const;

    bool collidesWithItem(const GraphicsItem* other,
                          Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    QList<GraphicsItem*> collidingItems(Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);
    void prepareGeometryChange();

    virtual void mousePressEvent(GraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(GraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(GraphicsSceneMouseEvent* event);
    virtual void mouseDoubleClickEvent(GraphicsSceneMouseEvent* event);
    virtual void hoverEnterEvent(GraphicsSceneHoverEvent* event);
    virtual void hoverMoveEvent(GraphicsSceneHoverEvent* event);
    virtual void hoverLeaveEvent(GraphicsSceneHoverEvent* event);
    virtual void contextMenuEvent(GraphicsSceneContextMenuEvent* event);
    virtual void dragEnterEvent(GraphicsSceneDragDropEvent* event);
    virtual void dragMoveEvent(GraphicsSceneDragDropEvent* event);
    virtual void dragLeaveEvent(GraphicsSceneDragDropEvent* event);
    virtual void dropEvent(GraphicsSceneDragDropEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

private:
    friend class GraphicsScene;
    void setSceneRecursive(GraphicsScene* scene);

private:
    GraphicsScene* m_scene;
    GraphicsItem* m_parent;
    QList<GraphicsItem*> m_children;
    QPointF m_pos;
    qreal m_scale;
    qreal m_z;
    bool m_visible;
    bool m_enabled;
    bool m_selected;
    bool m_acceptHover;
    bool m_acceptDrops;
    bool m_hasCursor;
    Qt::MouseButtons m_acceptedButtons;
    GraphicsItemFlags m_flags;
    QCursor m_cursor;
    QMap<int, QVariant> m_data;
    quint64 m_insertionOrder;
};

class GraphicsRectItem: public GraphicsItem {
public:
    explicit GraphicsRectItem(GraphicsItem* parent = nullptr): GraphicsItem(parent) { }
    GraphicsRectItem(const QRectF& rect, GraphicsItem* parent = nullptr):
        GraphicsItem(parent),
        m_rect(rect)
    {
    }
    QRectF rect() const { return m_rect; }
    void setRect(const QRectF& rect)
    {
        prepareGeometryChange();
        m_rect = rect;
        update();
    }
    void setRect(qreal x, qreal y, qreal w, qreal h) { setRect(QRectF(x, y, w, h)); }
    QPen pen() const { return m_pen; }
    void setPen(const QPen& pen)
    {
        m_pen = pen;
        update();
    }
    QBrush brush() const { return m_brush; }
    void setBrush(const QBrush& brush)
    {
        m_brush = brush;
        update();
    }
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const StyleOptionGraphicsItem* option) override;

private:
    QRectF m_rect;
    QPen m_pen;
    QBrush m_brush;
};

class GraphicsScene: public QObject {
    Q_OBJECT
public:
    explicit GraphicsScene(QObject* parent = nullptr);
    ~GraphicsScene();

    void addItem(GraphicsItem* item);
    void removeItem(GraphicsItem* item);
    GraphicsRectItem* addRect(qreal x, qreal y, qreal w, qreal h, const QPen& pen = QPen(),
                              const QBrush& brush = QBrush());

    // Items in descending stacking order (topmost first), like QGraphicsScene.
    QList<GraphicsItem*> items(Qt::SortOrder order = Qt::DescendingOrder) const;
    QList<GraphicsItem*> items(const QPointF& pos) const;
    QList<GraphicsItem*> items(const QRectF& rect,
                               Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    GraphicsItem* itemAt(const QPointF& pos,
                         const QTransform& deviceTransform = QTransform()) const;
    QList<GraphicsItem*> collidingItems(const GraphicsItem* item,
                                        Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;

    QList<GraphicsItem*> selectedItems() const;
    void clearSelection();

    QRectF sceneRect() const;
    void setSceneRect(const QRectF& rect);
    void setSceneRect(qreal x, qreal y, qreal w, qreal h) { setSceneRect(QRectF(x, y, w, h)); }
    qreal width() const { return sceneRect().width(); }
    qreal height() const { return sceneRect().height(); }
    QRectF itemsBoundingRect() const;

    QBrush backgroundBrush() const { return m_backgroundBrush; }
    void setBackgroundBrush(const QBrush& brush)
    {
        m_backgroundBrush = brush;
        update();
    }

    GraphicsItem* focusItem() const { return m_focusItem; }
    void setFocusItem(GraphicsItem* item);
    GraphicsItem* mouseGrabberItem() const { return m_mouseGrabber; }

    void render(QPainter* painter, const QRectF& target = QRectF(), const QRectF& source = QRectF(),
                Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);

    // Entry points used by a hosting view (e.g. the Qt Quick canvas).
    // All positions are in scene coordinates, screenPos in global coordinates.
    void handleMousePress(const QPointF& scenePos, const QPointF& screenPos, Qt::MouseButton button,
                          Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseMove(const QPointF& scenePos, const QPointF& screenPos,
                         Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseRelease(const QPointF& scenePos, const QPointF& screenPos,
                            Qt::MouseButton button, Qt::MouseButtons buttons,
                            Qt::KeyboardModifiers modifiers);
    void handleMouseDoubleClick(const QPointF& scenePos, const QPointF& screenPos,
                                Qt::MouseButton button, Qt::MouseButtons buttons,
                                Qt::KeyboardModifiers modifiers);
    void handleHoverLeave();
    void handleContextMenu(const QPointF& scenePos, const QPoint& screenPos,
                           Qt::KeyboardModifiers modifiers);
    void handleKeyPress(QKeyEvent* event);
    void handleKeyRelease(QKeyEvent* event);
    bool handleDragEnter(const QPointF& scenePos, const QMimeData* mimeData);
    void handleDragMove(const QPointF& scenePos, const QMimeData* mimeData);
    void handleDragLeave();
    bool handleDrop(const QPointF& scenePos, const QMimeData* mimeData);
    QCursor cursorAt(const QPointF& scenePos) const;

    // Asks the hosting view to show a context menu. The menu is owned by the
    // scene and deleted after it has been shown.
    void showPopupMenu(PopupMenu* menu, const QPointF& scenePos);

public slots:
    void update(const QRectF& rect = QRectF());
    void invalidate(const QRectF& rect = QRectF()) { update(rect); }

signals:
    void changed();
    void selectionChanged();
    void sceneRectChanged(const QRectF& rect);
    void cursorChanged();
    void popupMenuRequested(LimeReport::PopupMenu* menu, const QPointF& scenePos);

protected:
    virtual void mousePressEvent(GraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(GraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(GraphicsSceneMouseEvent* event);
    virtual void mouseDoubleClickEvent(GraphicsSceneMouseEvent* event);
    virtual void contextMenuEvent(GraphicsSceneContextMenuEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void dragEnterEvent(GraphicsSceneDragDropEvent* event);
    virtual void dragMoveEvent(GraphicsSceneDragDropEvent* event);
    virtual void dragLeaveEvent(GraphicsSceneDragDropEvent* event);
    virtual void dropEvent(GraphicsSceneDragDropEvent* event);

private:
    friend class GraphicsItem;
    void itemDestroyed(GraphicsItem* item, bool wasSelected);
    void itemSelectionChanged();
    void collectItems(GraphicsItem* item, QList<GraphicsItem*>& list) const;
    void collectItemsAt(GraphicsItem* item, const QPointF& scenePos, const QPainterPath* clip,
                        QList<GraphicsItem*>& list) const;
    QList<GraphicsItem*> sortedTopLevelItems() const;
    void drawItem(QPainter* painter, GraphicsItem* item, const QTransform& parentSceneTransform,
                  const QTransform& viewTransform);
    void prepareMouseEvent(GraphicsSceneMouseEvent& event, const QPointF& scenePos,
                           const QPointF& screenPos, Qt::MouseButton button,
                           Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void dispatchHover(const QPointF& scenePos, const QPointF& screenPos,
                       Qt::KeyboardModifiers modifiers);
    void pressHandler(GraphicsSceneMouseEvent* event, bool doubleClick);
    void startMoveTracking(GraphicsItem* item);
    void moveSelectedItems(GraphicsItem* item, GraphicsSceneMouseEvent* event);
    void setHoverItems(const QList<GraphicsItem*>& items, const QPointF& scenePos,
                       const QPointF& screenPos, Qt::KeyboardModifiers modifiers);

private:
    QList<GraphicsItem*> m_topLevelItems;
    QRectF m_sceneRect;
    bool m_hasSceneRect;
    QBrush m_backgroundBrush;
    GraphicsItem* m_focusItem;
    GraphicsItem* m_mouseGrabber;
    GraphicsItem* m_lastMouseGrabber;
    QList<GraphicsItem*> m_hoverItems;
    GraphicsItem* m_dragOverItem;
    int m_selectionChanging;
    bool m_selectionDirty;
    bool m_updatePending;
    quint64 m_insertionCounter;
    QMap<Qt::MouseButton, QPointF> m_buttonDownScenePos;
    QMap<Qt::MouseButton, QPointF> m_buttonDownScreenPos;
    QPointF m_lastScenePos;
    QPointF m_lastScreenPos;
    QMap<GraphicsItem*, QPointF> m_movingItemsInitialPositions;
};

} // namespace LimeReport

Q_DECLARE_OPERATORS_FOR_FLAGS(LimeReport::GraphicsItem::GraphicsItemFlags)
Q_DECLARE_METATYPE(LimeReport::GraphicsItem*)

#endif // LRGRAPHICSSCENE_H
