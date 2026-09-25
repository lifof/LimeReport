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
#include "lrgraphicsscene.h"

#include <QPainter>
#include <QPaintDevice>
#include <QKeyEvent>
#include <QMimeData>
#include <QAction>
#include <QTimer>
#include <QMetaMethod>
#include <algorithm>

namespace LimeReport {

// ---------------------------------------------------------------------------
// PopupMenu
// ---------------------------------------------------------------------------

QAction* PopupMenu::addAction(const QString& text)
{
    QAction* action = new QAction(text, this);
    m_actions.append(action);
    emit actionsChanged();
    return action;
}

QAction* PopupMenu::addAction(const QIcon& icon, const QString& text)
{
    QAction* action = new QAction(icon, text, this);
    m_actions.append(action);
    emit actionsChanged();
    return action;
}

QAction* PopupMenu::addSeparator()
{
    QAction* action = new QAction(this);
    action->setSeparator(true);
    m_actions.append(action);
    emit actionsChanged();
    return action;
}

void PopupMenu::insertAction(QAction* before, QAction* action)
{
    m_actions.removeAll(action);
    int index = m_actions.indexOf(before);
    if (index < 0) m_actions.append(action); else m_actions.insert(index, action);
    emit actionsChanged();
}

QAction* PopupMenu::insertSeparator(QAction* before)
{
    QAction* action = new QAction(this);
    action->setSeparator(true);
    insertAction(before, action);
    return action;
}

QList<QObject*> PopupMenu::actionObjects() const
{
    QList<QObject*> result;
    for (QAction* action : m_actions) result.append(action);
    return result;
}

QVariantList PopupMenu::items() const
{
    QVariantList result;
    for (QAction* action : m_actions) {
        QVariantMap item;
        item.insert("text", action->text());
        item.insert("enabled", action->isEnabled());
        item.insert("visible", action->isVisible());
        item.insert("checkable", action->isCheckable());
        item.insert("checked", action->isChecked());
        item.insert("separator", action->isSeparator());
        item.insert("shortcut", action->shortcut().toString(QKeySequence::NativeText));
        result.append(item);
    }
    return result;
}

void PopupMenu::trigger(int index)
{
    if (index < 0 || index >= m_actions.size()) return;
    QAction* action = m_actions.at(index);
    if (action->isSeparator() || !action->isEnabled()) return;
    action->trigger();
    emit triggered(action);
}

// ---------------------------------------------------------------------------
// GraphicsItem
// ---------------------------------------------------------------------------

static bool zLessThan(const GraphicsItem* a, const GraphicsItem* b);

GraphicsItem::GraphicsItem(GraphicsItem* parent)
    : m_scene(nullptr), m_parent(nullptr), m_scale(1), m_z(0), m_visible(true), m_enabled(true),
      m_selected(false), m_acceptHover(false), m_acceptDrops(false), m_hasCursor(false),
      m_acceptedButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton),
      m_insertionOrder(0)
{
    static quint64 counter = 0;
    m_insertionOrder = ++counter;
    if (parent && parent != this) {
        // No virtual calls on this object here: it is not fully constructed yet.
        m_parent = parent;
        parent->m_children.append(this);
        if (parent->m_scene) setSceneRecursive(parent->m_scene);
        parent->itemChange(ItemChildAddedChange, QVariant::fromValue<GraphicsItem*>(this));
        parent->update();
    }
}

GraphicsItem::~GraphicsItem()
{
    GraphicsScene* scene = m_scene;
    bool wasSelected = m_selected;
    m_selected = false;
    // Detach first: the derived parts of this object are already destroyed,
    // so nothing may reach it through the tree while notifications are sent.
    if (m_parent) {
        m_parent->m_children.removeAll(this);
        m_parent = nullptr;
    } else if (scene) {
        scene->m_topLevelItems.removeAll(this);
    }
    while (!m_children.isEmpty()) {
        GraphicsItem* child = m_children.takeFirst();
        child->m_parent = nullptr;
        delete child;
    }
    m_scene = nullptr;
    if (scene) scene->itemDestroyed(this, wasSelected);
}

QPainterPath GraphicsItem::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

bool GraphicsItem::contains(const QPointF& point) const
{
    return shape().contains(point);
}

GraphicsItem* GraphicsItem::topLevelItem() const
{
    const GraphicsItem* item = this;
    while (item->m_parent) item = item->m_parent;
    return const_cast<GraphicsItem*>(item);
}

bool GraphicsItem::isAncestorOf(const GraphicsItem* child) const
{
    if (!child) return false;
    const GraphicsItem* p = child->m_parent;
    while (p) {
        if (p == this) return true;
        p = p->m_parent;
    }
    return false;
}

QList<GraphicsItem*> GraphicsItem::childItems() const
{
    QList<GraphicsItem*> result = m_children;
    std::stable_sort(result.begin(), result.end(), zLessThan);
    return result;
}

void GraphicsItem::setSceneRecursive(GraphicsScene* scene)
{
    if (m_scene == scene) return;
    QVariant v = itemChange(ItemSceneChange, QVariant::fromValue<QObject*>(scene));
    Q_UNUSED(v)
    if (m_scene) {
        GraphicsScene* old = m_scene;
        bool wasSelected = m_selected;
        m_selected = false;
        old->itemDestroyed(this, wasSelected);
    }
    m_scene = scene;
    foreach (GraphicsItem* child, m_children) child->setSceneRecursive(scene);
    itemChange(ItemSceneHasChanged, QVariant::fromValue<QObject*>(scene));
}

void GraphicsItem::setParentItem(GraphicsItem* newParent)
{
    if (newParent == m_parent) return;
    if (newParent == this || isAncestorOf(newParent)) return;
    QVariant v = itemChange(ItemParentChange, QVariant::fromValue<GraphicsItem*>(newParent));
    newParent = v.value<GraphicsItem*>();
    if (newParent == m_parent) return;

    update();
    if (m_parent) {
        GraphicsItem* oldParent = m_parent;
        oldParent->itemChange(ItemChildRemovedChange, QVariant::fromValue<GraphicsItem*>(this));
        oldParent->m_children.removeAll(this);
        oldParent->update();
    } else if (m_scene) {
        m_scene->m_topLevelItems.removeAll(this);
    }

    m_parent = newParent;
    if (newParent) {
        newParent->m_children.append(this);
        if (newParent->m_scene != m_scene) {
            GraphicsScene* target = newParent->m_scene;
            setSceneRecursive(target);
        }
        newParent->itemChange(ItemChildAddedChange, QVariant::fromValue<GraphicsItem*>(this));
    } else if (m_scene) {
        m_scene->m_topLevelItems.append(this);
    }
    itemChange(ItemParentHasChanged, QVariant::fromValue<GraphicsItem*>(newParent));
    update();
}

void GraphicsItem::setPos(const QPointF& pos)
{
    QPointF newPos = pos;
    if (m_flags & ItemSendsGeometryChanges) {
        newPos = itemChange(ItemPositionChange, newPos).toPointF();
    }
    if (newPos == m_pos) return;
    update();
    m_pos = newPos;
    update();
    if (m_flags & ItemSendsGeometryChanges)
        itemChange(ItemPositionHasChanged, m_pos);
}

QPointF GraphicsItem::scenePos() const
{
    return mapToScene(QPointF(0, 0));
}

void GraphicsItem::setScale(qreal scale)
{
    qreal newScale = itemChange(ItemScaleChange, scale).toReal();
    if (qFuzzyCompare(newScale, m_scale)) return;
    update();
    m_scale = newScale;
    update();
    itemChange(ItemScaleHasChanged, m_scale);
}

void GraphicsItem::setZValue(qreal z)
{
    qreal newZ = itemChange(ItemZValueChange, z).toReal();
    if (newZ == m_z) return;
    m_z = newZ;
    update();
    itemChange(ItemZValueHasChanged, m_z);
}

bool GraphicsItem::isVisible() const
{
    if (!m_visible) return false;
    return m_parent ? m_parent->isVisible() : true;
}

void GraphicsItem::setVisible(bool visible)
{
    if (m_visible == visible) return;
    visible = itemChange(ItemVisibleChange, visible).toBool();
    if (m_visible == visible) return;
    if (!visible) update();
    m_visible = visible;
    if (!visible) {
        if (m_scene) {
            if (m_scene->m_mouseGrabber == this || isAncestorOf(m_scene->m_mouseGrabber))
                m_scene->m_mouseGrabber = nullptr;
            if (hasFocus()) clearFocus();
        }
        if (m_selected) setSelected(false);
    } else {
        update();
    }
    itemChange(ItemVisibleHasChanged, visible);
}

void GraphicsItem::setSelected(bool selected)
{
    if (m_selected == selected) return;
    if (selected && (!(m_flags & ItemIsSelectable) || !isVisible() || !m_enabled)) return;
    bool newValue = itemChange(ItemSelectedChange, selected).toBool();
    if (m_selected == newValue) return;
    m_selected = newValue;
    update();
    if (m_scene) m_scene->itemSelectionChanged();
    itemChange(ItemSelectedHasChanged, m_selected);
}

void GraphicsItem::setFlag(GraphicsItemFlag flag, bool enabled)
{
    GraphicsItemFlags f = m_flags;
    if (enabled) f |= flag; else f &= ~flag;
    setFlags(f);
}

void GraphicsItem::setFlags(GraphicsItemFlags flags)
{
    if (m_flags == flags) return;
    m_flags = flags;
    if (!(m_flags & ItemIsSelectable) && m_selected) setSelected(false);
    update();
}

void GraphicsItem::setCursor(const QCursor& cursor)
{
    itemChange(ItemCursorChange, QVariant::fromValue(cursor));
    m_cursor = cursor;
    m_hasCursor = true;
    itemChange(ItemCursorHasChanged, QVariant::fromValue(cursor));
    if (m_scene) emit m_scene->cursorChanged();
}

void GraphicsItem::unsetCursor()
{
    m_hasCursor = false;
    m_cursor = QCursor();
    if (m_scene) emit m_scene->cursorChanged();
}

bool GraphicsItem::hasFocus() const
{
    return m_scene && m_scene->m_focusItem == this;
}

void GraphicsItem::setFocus()
{
    if (m_scene) m_scene->setFocusItem(this);
}

void GraphicsItem::clearFocus()
{
    if (m_scene && m_scene->m_focusItem == this) m_scene->setFocusItem(nullptr);
}

void GraphicsItem::update(const QRectF& rect)
{
    if (!m_scene) return;
    QRectF r = rect.isNull() ? boundingRect() : rect;
    m_scene->update(mapRectToScene(r));
}

void GraphicsItem::prepareGeometryChange()
{
    update();
}

QTransform GraphicsItem::transform() const
{
    QTransform t;
    t.translate(m_pos.x(), m_pos.y());
    if (m_scale != 1) t.scale(m_scale, m_scale);
    return t;
}

QTransform GraphicsItem::sceneTransform() const
{
    QTransform t = transform();
    if (m_parent) t = t * m_parent->sceneTransform();
    return t;
}

QRectF GraphicsItem::sceneBoundingRect() const
{
    return sceneTransform().mapRect(boundingRect());
}

QPointF GraphicsItem::mapToScene(const QPointF& point) const
{
    return sceneTransform().map(point);
}

QPolygonF GraphicsItem::mapToScene(const QRectF& rect) const
{
    return sceneTransform().map(QPolygonF(rect));
}

QPainterPath GraphicsItem::mapToScene(const QPainterPath& path) const
{
    return sceneTransform().map(path);
}

QPointF GraphicsItem::mapFromScene(const QPointF& point) const
{
    return sceneTransform().inverted().map(point);
}

QPolygonF GraphicsItem::mapFromScene(const QRectF& rect) const
{
    return sceneTransform().inverted().map(QPolygonF(rect));
}

QPointF GraphicsItem::mapToParent(const QPointF& point) const
{
    return transform().map(point);
}

QPointF GraphicsItem::mapFromParent(const QPointF& point) const
{
    return transform().inverted().map(point);
}

QPointF GraphicsItem::mapToItem(const GraphicsItem* item, const QPointF& point) const
{
    QPointF scenePoint = mapToScene(point);
    return item ? item->mapFromScene(scenePoint) : scenePoint;
}

QPointF GraphicsItem::mapFromItem(const GraphicsItem* item, const QPointF& point) const
{
    QPointF scenePoint = item ? item->mapToScene(point) : point;
    return mapFromScene(scenePoint);
}

QRectF GraphicsItem::mapRectToScene(const QRectF& rect) const
{
    return sceneTransform().mapRect(rect);
}

QRectF GraphicsItem::mapRectFromScene(const QRectF& rect) const
{
    return sceneTransform().inverted().mapRect(rect);
}

QRectF GraphicsItem::mapRectToParent(const QRectF& rect) const
{
    return transform().mapRect(rect);
}

QRectF GraphicsItem::mapRectFromParent(const QRectF& rect) const
{
    return transform().inverted().mapRect(rect);
}

bool GraphicsItem::collidesWithItem(const GraphicsItem* other, Qt::ItemSelectionMode mode) const
{
    if (!other || other == this) return false;
    if (mode == Qt::IntersectsItemBoundingRect || mode == Qt::ContainsItemBoundingRect) {
        QRectF a = sceneBoundingRect();
        QRectF b = other->sceneBoundingRect();
        return mode == Qt::IntersectsItemBoundingRect ? a.intersects(b) : a.contains(b);
    }
    QPainterPath a = mapToScene(shape());
    QPainterPath b = other->mapToScene(other->shape());
    return mode == Qt::IntersectsItemShape ? a.intersects(b) : a.contains(b);
}

QList<GraphicsItem*> GraphicsItem::collidingItems(Qt::ItemSelectionMode mode) const
{
    if (!m_scene) return QList<GraphicsItem*>();
    return m_scene->collidingItems(this, mode);
}

QVariant GraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    Q_UNUSED(change)
    return value;
}

void GraphicsItem::mousePressEvent(GraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && (m_flags & ItemIsSelectable)) {
        bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;
        if (!multiSelect && !m_selected) {
            if (m_scene) {
                ++m_scene->m_selectionChanging;
                m_scene->clearSelection();
                --m_scene->m_selectionChanging;
            }
            setSelected(true);
            if (m_scene && m_scene->m_selectionChanging == 0 && m_scene->m_selectionDirty) {
                m_scene->m_selectionDirty = false;
                emit m_scene->selectionChanged();
            }
        }
    } else if (!(m_flags & ItemIsMovable)) {
        event->ignore();
    }
}

void GraphicsItem::mouseMoveEvent(GraphicsSceneMouseEvent* event)
{
    if ((event->buttons() & Qt::LeftButton) && (m_flags & ItemIsMovable) && m_scene) {
        m_scene->moveSelectedItems(this, event);
    } else {
        event->ignore();
    }
}

void GraphicsItem::mouseReleaseEvent(GraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && (m_flags & ItemIsSelectable)) {
        bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;
        if (event->scenePos() == event->buttonDownScenePos(Qt::LeftButton)) {
            if (multiSelect) {
                setSelected(!m_selected);
            } else if (m_scene) {
                ++m_scene->m_selectionChanging;
                bool wasSelected = m_selected;
                foreach (GraphicsItem* item, m_scene->selectedItems()) {
                    if (item != this) item->setSelected(false);
                }
                if (!wasSelected) setSelected(true);
                --m_scene->m_selectionChanging;
                if (m_scene->m_selectionChanging == 0 && m_scene->m_selectionDirty) {
                    m_scene->m_selectionDirty = false;
                    emit m_scene->selectionChanged();
                }
            } else {
                setSelected(true);
            }
        }
    }
}

void GraphicsItem::mouseDoubleClickEvent(GraphicsSceneMouseEvent* event)
{
    mousePressEvent(event);
}

void GraphicsItem::hoverEnterEvent(GraphicsSceneHoverEvent*) { update(); }
void GraphicsItem::hoverMoveEvent(GraphicsSceneHoverEvent*) {}
void GraphicsItem::hoverLeaveEvent(GraphicsSceneHoverEvent*) { update(); }
void GraphicsItem::contextMenuEvent(GraphicsSceneContextMenuEvent* event) { event->ignore(); }
void GraphicsItem::dragEnterEvent(GraphicsSceneDragDropEvent* event) { event->setAccepted(m_acceptDrops); }
void GraphicsItem::dragMoveEvent(GraphicsSceneDragDropEvent* event) { event->setAccepted(m_acceptDrops); }
void GraphicsItem::dragLeaveEvent(GraphicsSceneDragDropEvent*) {}
void GraphicsItem::dropEvent(GraphicsSceneDragDropEvent* event) { event->ignore(); }
void GraphicsItem::keyPressEvent(QKeyEvent* event) { event->ignore(); }
void GraphicsItem::keyReleaseEvent(QKeyEvent* event) { event->ignore(); }

static bool zLessThan(const GraphicsItem* a, const GraphicsItem* b)
{
    if (a->zValue() != b->zValue()) return a->zValue() < b->zValue();
    return false; // stable_sort keeps insertion order
}

// ---------------------------------------------------------------------------
// GraphicsRectItem
// ---------------------------------------------------------------------------

QRectF GraphicsRectItem::boundingRect() const
{
    qreal pw = m_pen.style() == Qt::NoPen ? 0 : m_pen.widthF() / 2 + 1;
    return m_rect.adjusted(-pw, -pw, pw, pw);
}

void GraphicsRectItem::paint(QPainter* painter, const StyleOptionGraphicsItem*)
{
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    painter->drawRect(m_rect);
}

// ---------------------------------------------------------------------------
// GraphicsScene
// ---------------------------------------------------------------------------

GraphicsScene::GraphicsScene(QObject* parent)
    : QObject(parent), m_hasSceneRect(false), m_focusItem(nullptr), m_mouseGrabber(nullptr),
      m_lastMouseGrabber(nullptr), m_dragOverItem(nullptr), m_selectionChanging(0),
      m_selectionDirty(false), m_updatePending(false), m_insertionCounter(0)
{
}

GraphicsScene::~GraphicsScene()
{
    // Like QGraphicsScene, the scene owns and deletes its top-level items.
    m_hoverItems.clear();
    m_mouseGrabber = nullptr;
    m_focusItem = nullptr;
    while (!m_topLevelItems.isEmpty()) {
        GraphicsItem* item = m_topLevelItems.takeFirst();
        item->setSceneRecursive(nullptr);
        delete item;
    }
}

void GraphicsScene::addItem(GraphicsItem* item)
{
    if (!item || item->m_scene == this) return;
    if (item->m_scene) item->m_scene->removeItem(item);
    if (item->m_parent && item->m_parent->m_scene != this) item->setParentItem(nullptr);
    item->setSceneRecursive(this);
    if (!item->m_parent) m_topLevelItems.append(item);
    item->update();
}

void GraphicsScene::removeItem(GraphicsItem* item)
{
    if (!item || item->m_scene != this) return;
    item->update();
    if (item->m_parent) {
        item->setParentItem(nullptr);
    }
    m_topLevelItems.removeAll(item);
    item->setSceneRecursive(nullptr);
}

GraphicsRectItem* GraphicsScene::addRect(qreal x, qreal y, qreal w, qreal h, const QPen& pen, const QBrush& brush)
{
    GraphicsRectItem* item = new GraphicsRectItem(QRectF(x, y, w, h));
    item->setPen(pen);
    item->setBrush(brush);
    addItem(item);
    return item;
}

void GraphicsScene::itemDestroyed(GraphicsItem* item, bool wasSelected)
{
    m_hoverItems.removeAll(item);
    if (m_mouseGrabber == item) m_mouseGrabber = nullptr;
    if (m_lastMouseGrabber == item) m_lastMouseGrabber = nullptr;
    if (m_focusItem == item) m_focusItem = nullptr;
    if (m_dragOverItem == item) m_dragOverItem = nullptr;
    m_movingItemsInitialPositions.remove(item);
    if (wasSelected) itemSelectionChanged();
    update();
}

void GraphicsScene::itemSelectionChanged()
{
    if (m_selectionChanging > 0) {
        m_selectionDirty = true;
    } else {
        emit selectionChanged();
    }
}

QList<GraphicsItem*> GraphicsScene::sortedTopLevelItems() const
{
    QList<GraphicsItem*> result = m_topLevelItems;
    std::stable_sort(result.begin(), result.end(), zLessThan);
    return result;
}

void GraphicsScene::collectItems(GraphicsItem* item, QList<GraphicsItem*>& list) const
{
    QList<GraphicsItem*> children = item->childItems();
    QList<GraphicsItem*> behind;
    QList<GraphicsItem*> front;
    foreach (GraphicsItem* child, children) {
        if ((child->m_flags & GraphicsItem::ItemStacksBehindParent) ||
            (child->m_z < 0 && (child->m_flags & GraphicsItem::ItemNegativeZStacksBehindParent)))
            behind.append(child);
        else
            front.append(child);
    }
    foreach (GraphicsItem* child, behind) collectItems(child, list);
    list.append(item);
    foreach (GraphicsItem* child, front) collectItems(child, list);
}

QList<GraphicsItem*> GraphicsScene::items(Qt::SortOrder order) const
{
    QList<GraphicsItem*> result;
    foreach (GraphicsItem* item, sortedTopLevelItems()) collectItems(item, result);
    if (order == Qt::DescendingOrder) std::reverse(result.begin(), result.end());
    return result;
}

void GraphicsScene::collectItemsAt(GraphicsItem* item, const QPointF& scenePos, const QPainterPath* clip,
                                   QList<GraphicsItem*>& list) const
{
    if (!item->isVisible()) return;
    QPointF local = item->mapFromScene(scenePos);
    bool inClip = !clip || clip->contains(scenePos);

    QPainterPath childClip;
    const QPainterPath* childClipPtr = clip;
    if (item->m_flags & GraphicsItem::ItemClipsChildrenToShape) {
        childClip = item->mapToScene(item->shape());
        if (clip) childClip = childClip.intersected(*clip);
        childClipPtr = &childClip;
    }

    QList<GraphicsItem*> behind;
    QList<GraphicsItem*> front;
    foreach (GraphicsItem* child, item->childItems()) {
        if ((child->m_flags & GraphicsItem::ItemStacksBehindParent) ||
            (child->m_z < 0 && (child->m_flags & GraphicsItem::ItemNegativeZStacksBehindParent)))
            behind.append(child);
        else
            front.append(child);
    }
    foreach (GraphicsItem* child, behind) collectItemsAt(child, scenePos, childClipPtr, list);
    if (inClip && item->contains(local)) list.append(item);
    foreach (GraphicsItem* child, front) collectItemsAt(child, scenePos, childClipPtr, list);
}

QList<GraphicsItem*> GraphicsScene::items(const QPointF& pos) const
{
    QList<GraphicsItem*> result;
    foreach (GraphicsItem* item, sortedTopLevelItems()) collectItemsAt(item, pos, nullptr, result);
    std::reverse(result.begin(), result.end());
    return result;
}

QList<GraphicsItem*> GraphicsScene::items(const QRectF& rect, Qt::ItemSelectionMode mode) const
{
    QList<GraphicsItem*> result;
    QPainterPath area;
    area.addRect(rect);
    foreach (GraphicsItem* item, items()) {
        if (!item->isVisible()) continue;
        switch (mode) {
        case Qt::ContainsItemShape:
            if (area.contains(item->mapToScene(item->shape()))) result.append(item);
            break;
        case Qt::IntersectsItemShape:
            if (area.intersects(item->mapToScene(item->shape()))) result.append(item);
            break;
        case Qt::ContainsItemBoundingRect:
            if (rect.contains(item->sceneBoundingRect())) result.append(item);
            break;
        case Qt::IntersectsItemBoundingRect:
            if (rect.intersects(item->sceneBoundingRect())) result.append(item);
            break;
        }
    }
    return result;
}

GraphicsItem* GraphicsScene::itemAt(const QPointF& pos, const QTransform&) const
{
    QList<GraphicsItem*> list = items(pos);
    return list.isEmpty() ? nullptr : list.first();
}

QList<GraphicsItem*> GraphicsScene::collidingItems(const GraphicsItem* item, Qt::ItemSelectionMode mode) const
{
    QList<GraphicsItem*> result;
    if (!item) return result;
    foreach (GraphicsItem* other, items()) {
        if (other != item && other->isVisible() && item->collidesWithItem(other, mode))
            result.append(other);
    }
    return result;
}

QList<GraphicsItem*> GraphicsScene::selectedItems() const
{
    QList<GraphicsItem*> result;
    foreach (GraphicsItem* item, items(Qt::AscendingOrder)) {
        if (item->m_selected) result.append(item);
    }
    return result;
}

void GraphicsScene::clearSelection()
{
    ++m_selectionChanging;
    foreach (GraphicsItem* item, selectedItems()) item->setSelected(false);
    --m_selectionChanging;
    if (m_selectionChanging == 0 && m_selectionDirty) {
        m_selectionDirty = false;
        emit selectionChanged();
    }
}

QRectF GraphicsScene::sceneRect() const
{
    if (m_hasSceneRect) return m_sceneRect;
    return itemsBoundingRect();
}

void GraphicsScene::setSceneRect(const QRectF& rect)
{
    m_hasSceneRect = !rect.isNull();
    if (m_sceneRect != rect) {
        m_sceneRect = rect;
        emit sceneRectChanged(rect);
        update();
    }
}

QRectF GraphicsScene::itemsBoundingRect() const
{
    QRectF result;
    foreach (GraphicsItem* item, items()) result |= item->sceneBoundingRect();
    return result;
}

void GraphicsScene::setFocusItem(GraphicsItem* item)
{
    if (item && item->m_scene != this) return;
    m_focusItem = item;
}

void GraphicsScene::update(const QRectF&)
{
    if (m_updatePending) return;
    m_updatePending = true;
    QTimer::singleShot(0, this, [this]() {
        m_updatePending = false;
        emit changed();
    });
}

void GraphicsScene::drawItem(QPainter* painter, GraphicsItem* item, const QTransform& parentSceneTransform,
                             const QTransform& viewTransform)
{
    if (!item->m_visible) return;
    QTransform itemSceneTransform = item->transform() * parentSceneTransform;
    QTransform deviceTransform = itemSceneTransform * viewTransform;

    QList<GraphicsItem*> behind;
    QList<GraphicsItem*> front;
    foreach (GraphicsItem* child, item->childItems()) {
        if ((child->m_flags & GraphicsItem::ItemStacksBehindParent) ||
            (child->m_z < 0 && (child->m_flags & GraphicsItem::ItemNegativeZStacksBehindParent)))
            behind.append(child);
        else
            front.append(child);
    }

    painter->save();
    painter->setWorldTransform(deviceTransform);
    bool clipChildren = item->m_flags & GraphicsItem::ItemClipsChildrenToShape;
    if (clipChildren) painter->setClipPath(item->shape(), Qt::IntersectClip);

    foreach (GraphicsItem* child, behind) drawItem(painter, child, itemSceneTransform, viewTransform);

    StyleOptionGraphicsItem option;
    option.exposedRect = item->boundingRect();
    option.rect = option.exposedRect.toAlignedRect();
    if (item->m_selected) option.state |= StyleOptionGraphicsItem::State_Selected;
    if (m_hoverItems.contains(item)) option.state |= StyleOptionGraphicsItem::State_MouseOver;

    painter->save();
    painter->setWorldTransform(deviceTransform);
    if (item->m_flags & GraphicsItem::ItemClipsToShape) painter->setClipPath(item->shape(), Qt::IntersectClip);
    item->paint(painter, &option);
    painter->restore();

    foreach (GraphicsItem* child, front) drawItem(painter, child, itemSceneTransform, viewTransform);
    painter->restore();
}

void GraphicsScene::render(QPainter* painter, const QRectF& target, const QRectF& source,
                           Qt::AspectRatioMode aspectRatioMode)
{
    if (!painter) return;
    QRectF sourceRect = source.isNull() ? sceneRect() : source;
    QRectF targetRect = target;
    if (targetRect.isNull()) {
        if (painter->device())
            targetRect = QRectF(0, 0, painter->device()->width(), painter->device()->height());
        else
            targetRect = sourceRect;
    }
    if (sourceRect.isEmpty() || targetRect.isEmpty()) return;

    qreal xratio = targetRect.width() / sourceRect.width();
    qreal yratio = targetRect.height() / sourceRect.height();
    switch (aspectRatioMode) {
    case Qt::KeepAspectRatio:
        xratio = yratio = qMin(xratio, yratio);
        break;
    case Qt::KeepAspectRatioByExpanding:
        xratio = yratio = qMax(xratio, yratio);
        break;
    case Qt::IgnoreAspectRatio:
        break;
    }

    painter->save();
    QTransform viewTransform = painter->worldTransform();
    viewTransform.translate(targetRect.left(), targetRect.top());
    viewTransform.scale(xratio, yratio);
    viewTransform.translate(-sourceRect.left(), -sourceRect.top());

    painter->setClipRect(targetRect, Qt::IntersectClip);
    if (m_backgroundBrush.style() != Qt::NoBrush) {
        painter->save();
        painter->setWorldTransform(viewTransform);
        painter->fillRect(sourceRect, m_backgroundBrush);
        painter->restore();
    }
    foreach (GraphicsItem* item, sortedTopLevelItems())
        drawItem(painter, item, QTransform(), viewTransform);
    painter->restore();
}

// ----- input dispatch -------------------------------------------------------

void GraphicsScene::prepareMouseEvent(GraphicsSceneMouseEvent& event, const QPointF& scenePos, const QPointF& screenPos,
                                      Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    event.setScenePos(scenePos);
    event.setScreenPos(screenPos);
    event.setLastScenePos(m_lastScenePos);
    event.setButton(button);
    event.setButtons(buttons);
    event.setModifiers(modifiers);
    for (auto it = m_buttonDownScenePos.constBegin(); it != m_buttonDownScenePos.constEnd(); ++it)
        event.setButtonDownScenePos(it.key(), it.value());
}

static void mapMouseEventToItem(GraphicsSceneMouseEvent* event, GraphicsItem* item)
{
    event->setPos(item->mapFromScene(event->scenePos()));
    event->setLastPos(item->mapFromScene(event->lastScenePos()));
    foreach (Qt::MouseButton b, QList<Qt::MouseButton>() << Qt::LeftButton << Qt::RightButton << Qt::MiddleButton)
        event->setButtonDownPos(b, item->mapFromScene(event->buttonDownScenePos(b)));
}

void GraphicsScene::handleMousePress(const QPointF& scenePos, const QPointF& screenPos, Qt::MouseButton button,
                                     Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    m_buttonDownScenePos[button] = scenePos;
    m_buttonDownScreenPos[button] = screenPos;
    GraphicsSceneMouseEvent event;
    prepareMouseEvent(event, scenePos, screenPos, button, buttons, modifiers);
    mousePressEvent(&event);
    m_lastScenePos = scenePos;
    m_lastScreenPos = screenPos;
}

void GraphicsScene::handleMouseMove(const QPointF& scenePos, const QPointF& screenPos,
                                    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    GraphicsSceneMouseEvent event;
    prepareMouseEvent(event, scenePos, screenPos, Qt::NoButton, buttons, modifiers);
    mouseMoveEvent(&event);
    m_lastScenePos = scenePos;
    m_lastScreenPos = screenPos;
}

void GraphicsScene::handleMouseRelease(const QPointF& scenePos, const QPointF& screenPos, Qt::MouseButton button,
                                       Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    GraphicsSceneMouseEvent event;
    prepareMouseEvent(event, scenePos, screenPos, button, buttons, modifiers);
    mouseReleaseEvent(&event);
    m_lastScenePos = scenePos;
    m_lastScreenPos = screenPos;
    if (buttons == Qt::NoButton) dispatchHover(scenePos, screenPos, modifiers);
}

void GraphicsScene::handleMouseDoubleClick(const QPointF& scenePos, const QPointF& screenPos, Qt::MouseButton button,
                                           Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    m_buttonDownScenePos[button] = scenePos;
    m_buttonDownScreenPos[button] = screenPos;
    GraphicsSceneMouseEvent event;
    prepareMouseEvent(event, scenePos, screenPos, button, buttons, modifiers);
    mouseDoubleClickEvent(&event);
    m_lastScenePos = scenePos;
    m_lastScreenPos = screenPos;
}

void GraphicsScene::handleHoverLeave()
{
    setHoverItems(QList<GraphicsItem*>(), m_lastScenePos, m_lastScreenPos, Qt::NoModifier);
}

void GraphicsScene::handleContextMenu(const QPointF& scenePos, const QPoint& screenPos, Qt::KeyboardModifiers modifiers)
{
    GraphicsSceneContextMenuEvent event;
    event.setScenePos(scenePos);
    event.setScreenPos(screenPos);
    event.setModifiers(modifiers);
    contextMenuEvent(&event);
}

void GraphicsScene::handleKeyPress(QKeyEvent* event)
{
    keyPressEvent(event);
}

void GraphicsScene::handleKeyRelease(QKeyEvent* event)
{
    keyReleaseEvent(event);
}

bool GraphicsScene::handleDragEnter(const QPointF& scenePos, const QMimeData* mimeData)
{
    GraphicsSceneDragDropEvent event;
    event.setScenePos(scenePos);
    event.setMimeData(mimeData);
    event.ignore();
    dragEnterEvent(&event);
    return event.isAccepted();
}

void GraphicsScene::handleDragMove(const QPointF& scenePos, const QMimeData* mimeData)
{
    GraphicsSceneDragDropEvent event;
    event.setScenePos(scenePos);
    event.setMimeData(mimeData);
    dragMoveEvent(&event);
}

void GraphicsScene::handleDragLeave()
{
    GraphicsSceneDragDropEvent event;
    event.setScenePos(m_lastScenePos);
    dragLeaveEvent(&event);
}

bool GraphicsScene::handleDrop(const QPointF& scenePos, const QMimeData* mimeData)
{
    GraphicsSceneDragDropEvent event;
    event.setScenePos(scenePos);
    event.setMimeData(mimeData);
    event.ignore();
    dropEvent(&event);
    return event.isAccepted();
}

QCursor GraphicsScene::cursorAt(const QPointF& scenePos) const
{
    if (m_mouseGrabber && m_mouseGrabber->hasCursor()) return m_mouseGrabber->cursor();
    foreach (GraphicsItem* item, items(scenePos)) {
        if (item->hasCursor()) return item->cursor();
    }
    return QCursor(Qt::ArrowCursor);
}

void GraphicsScene::showPopupMenu(PopupMenu* menu, const QPointF& scenePos)
{
    if (!menu) return;
    menu->setParent(this);
    if (isSignalConnected(QMetaMethod::fromSignal(&GraphicsScene::popupMenuRequested))) {
        emit popupMenuRequested(menu, scenePos);
    } else {
        menu->deleteLater();
    }
}

void GraphicsScene::pressHandler(GraphicsSceneMouseEvent* event, bool doubleClick)
{
    if (m_mouseGrabber) {
        mapMouseEventToItem(event, m_mouseGrabber);
        if (doubleClick) m_mouseGrabber->mouseDoubleClickEvent(event);
        else m_mouseGrabber->mousePressEvent(event);
        return;
    }

    QList<GraphicsItem*> candidates = items(event->scenePos());

    bool focusSet = false;
    foreach (GraphicsItem* item, candidates) {
        if (item->isEnabled() && (item->m_flags & GraphicsItem::ItemIsFocusable)) {
            setFocusItem(item);
            focusSet = true;
            break;
        }
    }
    if (!focusSet) setFocusItem(nullptr);

    m_movingItemsInitialPositions.clear();
    foreach (GraphicsItem* item, candidates) {
        if (!(item->acceptedMouseButtons() & event->button())) continue;
        m_mouseGrabber = item;
        event->accept();
        mapMouseEventToItem(event, item);
        bool disabled = !item->isEnabled();
        if (doubleClick && item != m_lastMouseGrabber && m_lastMouseGrabber) {
            item->mousePressEvent(event);
        } else if (doubleClick) {
            item->mouseDoubleClickEvent(event);
        } else {
            item->mousePressEvent(event);
        }
        if (disabled) {
            m_mouseGrabber = nullptr;
            break;
        }
        if (event->isAccepted()) {
            m_lastMouseGrabber = m_mouseGrabber;
            return;
        }
        m_mouseGrabber = nullptr;
    }

    if (!event->isAccepted() || candidates.isEmpty()) {
        m_mouseGrabber = nullptr;
        event->ignore();
        if (!(event->modifiers() & Qt::ControlModifier)) clearSelection();
    }
}

void GraphicsScene::mousePressEvent(GraphicsSceneMouseEvent* event)
{
    pressHandler(event, false);
}

void GraphicsScene::mouseDoubleClickEvent(GraphicsSceneMouseEvent* event)
{
    pressHandler(event, true);
}

void GraphicsScene::mouseMoveEvent(GraphicsSceneMouseEvent* event)
{
    if (!m_mouseGrabber) {
        if (event->buttons() != Qt::NoButton) return;
        dispatchHover(event->scenePos(), event->screenPos(), event->modifiers());
        return;
    }
    mapMouseEventToItem(event, m_mouseGrabber);
    m_mouseGrabber->mouseMoveEvent(event);
    event->accept();
}

void GraphicsScene::mouseReleaseEvent(GraphicsSceneMouseEvent* event)
{
    if (!m_mouseGrabber) {
        event->ignore();
        return;
    }
    GraphicsItem* grabber = m_mouseGrabber;
    mapMouseEventToItem(event, grabber);
    grabber->mouseReleaseEvent(event);
    if (event->buttons() == Qt::NoButton) {
        m_mouseGrabber = nullptr;
        m_movingItemsInitialPositions.clear();
    }
}

void GraphicsScene::contextMenuEvent(GraphicsSceneContextMenuEvent* event)
{
    event->ignore();
    foreach (GraphicsItem* item, items(event->scenePos())) {
        event->setPos(item->mapFromScene(event->scenePos()));
        event->accept();
        item->contextMenuEvent(event);
        if (event->isAccepted()) break;
    }
}

void GraphicsScene::keyPressEvent(QKeyEvent* event)
{
    GraphicsItem* item = m_focusItem;
    event->ignore();
    while (item) {
        event->accept();
        item->keyPressEvent(event);
        if (event->isAccepted()) return;
        item = item->parentItem();
    }
}

void GraphicsScene::keyReleaseEvent(QKeyEvent* event)
{
    GraphicsItem* item = m_focusItem;
    event->ignore();
    while (item) {
        event->accept();
        item->keyReleaseEvent(event);
        if (event->isAccepted()) return;
        item = item->parentItem();
    }
}

void GraphicsScene::dragEnterEvent(GraphicsSceneDragDropEvent* event)
{
    m_dragOverItem = nullptr;
    dragMoveEvent(event);
}

void GraphicsScene::dragMoveEvent(GraphicsSceneDragDropEvent* event)
{
    event->ignore();
    foreach (GraphicsItem* item, items(event->scenePos())) {
        if (!item->acceptDrops()) continue;
        event->setPos(item->mapFromScene(event->scenePos()));
        if (item != m_dragOverItem) {
            if (m_dragOverItem) m_dragOverItem->dragLeaveEvent(event);
            m_dragOverItem = item;
            event->accept();
            item->dragEnterEvent(event);
        }
        event->accept();
        item->dragMoveEvent(event);
        if (event->isAccepted()) return;
    }
    if (m_dragOverItem) {
        m_dragOverItem->dragLeaveEvent(event);
        m_dragOverItem = nullptr;
    }
}

void GraphicsScene::dragLeaveEvent(GraphicsSceneDragDropEvent* event)
{
    if (m_dragOverItem) {
        m_dragOverItem->dragLeaveEvent(event);
        m_dragOverItem = nullptr;
    }
}

void GraphicsScene::dropEvent(GraphicsSceneDragDropEvent* event)
{
    if (m_dragOverItem) {
        event->setPos(m_dragOverItem->mapFromScene(event->scenePos()));
        event->accept();
        m_dragOverItem->dropEvent(event);
        m_dragOverItem = nullptr;
    } else {
        event->ignore();
    }
}

void GraphicsScene::dispatchHover(const QPointF& scenePos, const QPointF& screenPos, Qt::KeyboardModifiers modifiers)
{
    GraphicsItem* top = nullptr;
    foreach (GraphicsItem* item, items(scenePos)) {
        if (item->acceptHoverEvents()) {
            top = item;
            break;
        }
    }
    QList<GraphicsItem*> chain;
    GraphicsItem* item = top;
    while (item) {
        if (item->acceptHoverEvents()) chain.prepend(item);
        item = item->parentItem();
    }
    setHoverItems(chain, scenePos, screenPos, modifiers);
    if (top) {
        GraphicsSceneHoverEvent event;
        event.setScenePos(scenePos);
        event.setScreenPos(screenPos);
        event.setModifiers(modifiers);
        event.setPos(top->mapFromScene(scenePos));
        top->hoverMoveEvent(&event);
    }
}

void GraphicsScene::setHoverItems(const QList<GraphicsItem*>& newItems, const QPointF& scenePos,
                                  const QPointF& screenPos, Qt::KeyboardModifiers modifiers)
{
    GraphicsSceneHoverEvent event;
    event.setScenePos(scenePos);
    event.setScreenPos(screenPos);
    event.setModifiers(modifiers);
    QList<GraphicsItem*> old = m_hoverItems;
    m_hoverItems = newItems;
    for (int i = old.size() - 1; i >= 0; --i) {
        GraphicsItem* item = old.at(i);
        if (!newItems.contains(item) && item->m_scene == this) {
            event.setPos(item->mapFromScene(scenePos));
            item->hoverLeaveEvent(&event);
        }
    }
    foreach (GraphicsItem* item, newItems) {
        if (!old.contains(item)) {
            event.setPos(item->mapFromScene(scenePos));
            item->hoverEnterEvent(&event);
        }
    }
}

static bool movableAncestorIsSelected(const GraphicsItem* item)
{
    const GraphicsItem* parent = item->parentItem();
    while (parent) {
        if ((parent->flags() & GraphicsItem::ItemIsMovable) && parent->isSelected()) return true;
        parent = parent->parentItem();
    }
    return false;
}

void GraphicsScene::moveSelectedItems(GraphicsItem* source, GraphicsSceneMouseEvent* event)
{
    QList<GraphicsItem*> toMove;
    if (source->isSelected()) toMove = selectedItems(); else toMove << source;
    foreach (GraphicsItem* item, toMove) {
        if (!(item->flags() & GraphicsItem::ItemIsMovable) || movableAncestorIsSelected(item)) continue;
        if (!m_movingItemsInitialPositions.contains(item))
            m_movingItemsInitialPositions.insert(item, item->pos());
        GraphicsItem* parent = item->parentItem();
        QPointF current = parent ? parent->mapFromScene(event->scenePos()) : event->scenePos();
        QPointF down = parent ? parent->mapFromScene(event->buttonDownScenePos(Qt::LeftButton))
                              : event->buttonDownScenePos(Qt::LeftButton);
        item->setPos(m_movingItemsInitialPositions.value(item) + current - down);
    }
}

} // namespace LimeReport
