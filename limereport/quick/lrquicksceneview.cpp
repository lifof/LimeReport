#include "lrquicksceneview.h"

#include <QCursor>
#include <QMimeData>
#include <QPainter>
#include <QQuickWindow>
#include <QtMath>

namespace LimeReport {

SceneView::SceneView(QQuickItem* parent):
    QQuickPaintedItem(parent),
    m_zoom(1),
    m_contentX(0),
    m_contentY(0),
    m_interactive(false),
    m_backgroundColor(Qt::gray)
{
    setAntialiasing(true);
    setOpaquePainting(true);
    setFlag(ItemIsFocusScope, false);
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptHoverEvents(false);
}

void SceneView::setSceneObject(QObject* sceneObject)
{
    GraphicsScene* scene = qobject_cast<GraphicsScene*>(sceneObject);
    if (m_scene == scene)
        return;
    if (m_scene)
        disconnect(m_scene, nullptr, this, nullptr);
    m_scene = scene;
    if (m_scene) {
        connect(m_scene, &GraphicsScene::changed, this, &SceneView::onSceneChanged);
        connect(m_scene, &GraphicsScene::sceneRectChanged, this, &SceneView::onSceneRectChanged);
        connect(m_scene, &GraphicsScene::cursorChanged, this, &SceneView::onCursorChanged);
        connect(m_scene, &GraphicsScene::popupMenuRequested, this,
                &SceneView::onPopupMenuRequested);
        connect(m_scene, &QObject::destroyed, this, &SceneView::onSceneDestroyed);
    }
    emit sceneChanged();
    emit contentSizeChanged();
    clampContentPos();
    update();
}

void SceneView::onSceneDestroyed()
{
    m_scene = nullptr;
    emit sceneChanged();
    emit contentSizeChanged();
    update();
}

void SceneView::setZoom(qreal zoom)
{
    zoom = qBound<qreal>(0.05, zoom, 10);
    if (qFuzzyCompare(zoom, m_zoom))
        return;
    // keep the view centre stable while zooming
    QPointF centre = toScene(QPointF(width() / 2, height() / 2));
    m_zoom = zoom;
    emit zoomChanged();
    emit contentSizeChanged();
    QRectF r = sceneRect();
    setContentX((centre.x() - r.left()) * m_zoom - width() / 2);
    setContentY((centre.y() - r.top()) * m_zoom - height() / 2);
    clampContentPos();
    update();
}

void SceneView::setContentX(qreal value)
{
    qreal maxX = qMax<qreal>(0, contentWidth() - width());
    value = qBound<qreal>(0, value, maxX);
    if (qFuzzyCompare(value + 1, m_contentX + 1))
        return;
    m_contentX = value;
    emit contentXChanged();
    update();
}

void SceneView::setContentY(qreal value)
{
    qreal maxY = qMax<qreal>(0, contentHeight() - height());
    value = qBound<qreal>(0, value, maxY);
    if (qFuzzyCompare(value + 1, m_contentY + 1))
        return;
    m_contentY = value;
    emit contentYChanged();
    update();
}

QRectF SceneView::sceneRect() const { return m_scene ? m_scene->sceneRect() : QRectF(); }

qreal SceneView::contentWidth() const { return sceneRect().width() * m_zoom; }

qreal SceneView::contentHeight() const { return sceneRect().height() * m_zoom; }

void SceneView::setInteractive(bool value)
{
    if (m_interactive == value)
        return;
    m_interactive = value;
    setAcceptedMouseButtons(value ? Qt::AllButtons : Qt::NoButton);
    setAcceptHoverEvents(value);
    emit interactiveChanged();
}

void SceneView::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor == color)
        return;
    m_backgroundColor = color;
    emit backgroundColorChanged();
    update();
}

QPointF SceneView::toScene(const QPointF& viewPos) const
{
    QRectF r = sceneRect();
    qreal originX = contentWidth() < width() ? (width() - contentWidth()) / 2 : -m_contentX;
    qreal originY = contentHeight() < height() ? (height() - contentHeight()) / 2 : -m_contentY;
    return QPointF(r.left() + (viewPos.x() - originX) / m_zoom,
                   r.top() + (viewPos.y() - originY) / m_zoom);
}

QPointF SceneView::mapToScene(qreal x, qreal y) const { return toScene(QPointF(x, y)); }

QPointF SceneView::mapFromScene(qreal x, qreal y) const
{
    QRectF r = sceneRect();
    qreal originX = contentWidth() < width() ? (width() - contentWidth()) / 2 : -m_contentX;
    qreal originY = contentHeight() < height() ? (height() - contentHeight()) / 2 : -m_contentY;
    return QPointF(originX + (x - r.left()) * m_zoom, originY + (y - r.top()) * m_zoom);
}

QPointF SceneView::toGlobal(const QPointF& viewPos) const { return mapToGlobal(viewPos); }

void SceneView::fitWidth(qreal sceneWidth)
{
    if (sceneWidth <= 0)
        sceneWidth = sceneRect().width();
    if (sceneWidth <= 0 || width() <= 0)
        return;
    setZoom((width() - 4) / sceneWidth);
}

void SceneView::fitRect(const QRectF& rect)
{
    if (rect.isEmpty() || width() <= 0 || height() <= 0)
        return;
    setZoom(qMin((width() - 4) / rect.width(), (height() - 4) / rect.height()));
    scrollToScene(rect.left(), rect.top());
}

void SceneView::scrollToScene(qreal x, qreal y)
{
    QRectF r = sceneRect();
    setContentX((x - r.left()) * m_zoom);
    setContentY((y - r.top()) * m_zoom);
}

void SceneView::clampContentPos()
{
    setContentX(m_contentX);
    setContentY(m_contentY);
}

void SceneView::paint(QPainter* painter)
{
    painter->fillRect(QRectF(0, 0, width(), height()), m_backgroundColor);
    if (!m_scene)
        return;
    QRectF source = sceneRect();
    if (source.isEmpty())
        return;
    QPointF origin = mapFromScene(source.left(), source.top());
    QRectF target(origin, QSizeF(source.width() * m_zoom, source.height() * m_zoom));
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setClipRect(QRectF(0, 0, width(), height()));
    m_scene->render(painter, target, source, Qt::IgnoreAspectRatio);
}

void SceneView::onSceneChanged() { update(); }

void SceneView::onSceneRectChanged()
{
    emit contentSizeChanged();
    clampContentPos();
    update();
}

void SceneView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    clampContentPos();
}

void SceneView::onCursorChanged()
{
    if (m_interactive)
        updateCursor(m_lastScenePos);
}

void SceneView::updateCursor(const QPointF& scenePos)
{
    if (!m_scene)
        return;
    QCursor c = m_scene->cursorAt(scenePos);
    if (cursor().shape() != c.shape())
        setCursor(c);
}

void SceneView::onPopupMenuRequested(PopupMenu* menu, const QPointF& scenePos)
{
    QPointF viewPos = mapFromScene(scenePos.x(), scenePos.y());
    emit popupMenuRequested(menu, viewPos.x(), viewPos.y());
}

void SceneView::mousePressEvent(QMouseEvent* event)
{
    if (!m_interactive || !m_scene) {
        event->ignore();
        return;
    }
    forceActiveFocus(Qt::MouseFocusReason);
    QPointF scenePos = toScene(event->position());
    m_lastScenePos = scenePos;
    m_scene->handleMousePress(scenePos, event->globalPosition(), event->button(), event->buttons(),
                              event->modifiers());
    if (event->button() == Qt::RightButton)
        m_scene->handleContextMenu(scenePos, event->globalPosition().toPoint(), event->modifiers());
    emit clicked(scenePos.x(), scenePos.y());
    event->accept();
}

void SceneView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_interactive || !m_scene) {
        event->ignore();
        return;
    }
    QPointF scenePos = toScene(event->position());
    m_lastScenePos = scenePos;
    m_scene->handleMouseMove(scenePos, event->globalPosition(), event->buttons(),
                             event->modifiers());
    updateCursor(scenePos);

    // auto scroll while dragging outside of the view
    if (event->buttons() & Qt::LeftButton) {
        const qreal margin = 16;
        if (event->position().x() < margin)
            setContentX(m_contentX - 10);
        else if (event->position().x() > width() - margin)
            setContentX(m_contentX + 10);
        if (event->position().y() < margin)
            setContentY(m_contentY - 10);
        else if (event->position().y() > height() - margin)
            setContentY(m_contentY + 10);
    }
    event->accept();
}

void SceneView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_interactive || !m_scene) {
        event->ignore();
        return;
    }
    QPointF scenePos = toScene(event->position());
    m_lastScenePos = scenePos;
    m_scene->handleMouseRelease(scenePos, event->globalPosition(), event->button(),
                                event->buttons(), event->modifiers());
    updateCursor(scenePos);
    event->accept();
}

void SceneView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_interactive || !m_scene) {
        event->ignore();
        return;
    }
    QPointF scenePos = toScene(event->position());
    m_lastScenePos = scenePos;
    m_scene->handleMouseDoubleClick(scenePos, event->globalPosition(), event->button(),
                                    event->buttons(), event->modifiers());
    event->accept();
}

void SceneView::hoverMoveEvent(QHoverEvent* event)
{
    if (!m_interactive || !m_scene)
        return;
    QPointF scenePos = toScene(event->position());
    m_lastScenePos = scenePos;
    m_scene->handleMouseMove(scenePos, mapToGlobal(event->position()), Qt::NoButton,
                             event->modifiers());
    updateCursor(scenePos);
}

void SceneView::hoverLeaveEvent(QHoverEvent*)
{
    if (m_scene)
        m_scene->handleHoverLeave();
    unsetCursor();
}

void SceneView::keyPressEvent(QKeyEvent* event)
{
    if (!m_interactive || !m_scene) {
        event->ignore();
        return;
    }
    m_scene->handleKeyPress(event);
}

void SceneView::keyReleaseEvent(QKeyEvent* event)
{
    if (!m_interactive || !m_scene) {
        event->ignore();
        return;
    }
    m_scene->handleKeyRelease(event);
}

void SceneView::wheelEvent(QWheelEvent* event)
{
    QPoint delta = event->angleDelta();
    if (event->modifiers() & Qt::ControlModifier) {
        qreal factor = delta.y() > 0 ? 1.1 : 1 / 1.1;
        emit wheelZoomRequested(factor, event->position().x(), event->position().y());
        setZoom(m_zoom * factor);
    } else if (event->modifiers() & Qt::ShiftModifier) {
        setContentX(m_contentX - delta.y() / 2.0);
    } else {
        setContentX(m_contentX - delta.x() / 2.0);
        setContentY(m_contentY - delta.y() / 2.0);
    }
    event->accept();
}

bool SceneView::dragEnter(qreal x, qreal y, const QString& text)
{
    if (!m_scene)
        return false;
    QMimeData mime;
    mime.setText(text);
    return m_scene->handleDragEnter(toScene(QPointF(x, y)), &mime);
}

void SceneView::dragMove(qreal x, qreal y, const QString& text)
{
    if (!m_scene)
        return;
    QMimeData mime;
    mime.setText(text);
    m_scene->handleDragMove(toScene(QPointF(x, y)), &mime);
}

void SceneView::dragLeave()
{
    if (m_scene)
        m_scene->handleDragLeave();
}

bool SceneView::drop(qreal x, qreal y, const QString& text)
{
    if (!m_scene)
        return false;
    QMimeData mime;
    mime.setText(text);
    return m_scene->handleDrop(toScene(QPointF(x, y)), &mime);
}

} // namespace LimeReport
