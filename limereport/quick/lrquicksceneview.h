#ifndef LRQUICKSCENEVIEW_H
#define LRQUICKSCENEVIEW_H

#include "lrgraphicsscene.h"

#include <QPointer>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

namespace LimeReport {

/*
 * Qt Quick replacement for QGraphicsView: paints a LimeReport scene and,
 * when interactive, forwards mouse, hover, keyboard, wheel and drop events
 * to it.
 *
 * The item acts as a viewport: its own size is the visible area, the
 * scrolled position is given by contentX / contentY (in device independent
 * pixels, i.e. already multiplied by zoom). Put it next to a Flickable or
 * ScrollBars and bind contentX/contentY to scroll.
 */
class SceneView: public QQuickPaintedItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(ReportSceneView)
    Q_PROPERTY(QObject* scene READ sceneObject WRITE setSceneObject NOTIFY sceneChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(qreal contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(qreal contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentSizeChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentSizeChanged)
    Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)
    Q_PROPERTY(QRectF sceneRect READ sceneRect NOTIFY contentSizeChanged)
public:
    explicit SceneView(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    GraphicsScene* scene() const { return m_scene; }
    QObject* sceneObject() const { return m_scene; }
    void setSceneObject(QObject* scene);

    qreal zoom() const { return m_zoom; }
    void setZoom(qreal zoom);
    qreal contentX() const { return m_contentX; }
    void setContentX(qreal value);
    qreal contentY() const { return m_contentY; }
    void setContentY(qreal value);
    qreal contentWidth() const;
    qreal contentHeight() const;
    QRectF sceneRect() const;

    bool interactive() const { return m_interactive; }
    void setInteractive(bool value);
    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    Q_INVOKABLE QPointF mapToScene(qreal x, qreal y) const;
    Q_INVOKABLE QPointF mapFromScene(qreal x, qreal y) const;
    // Zoom so that the given scene width / rect fits the view.
    Q_INVOKABLE void fitWidth(qreal sceneWidth = -1);
    Q_INVOKABLE void fitRect(const QRectF& sceneRect);
    // Scrolls so that the scene point is at the top-left of the view.
    Q_INVOKABLE void scrollToScene(qreal x, qreal y);
    // Drop support (QML DropArea forwards here).
    Q_INVOKABLE bool dragEnter(qreal x, qreal y, const QString& text);
    Q_INVOKABLE void dragMove(qreal x, qreal y, const QString& text);
    Q_INVOKABLE void dragLeave();
    Q_INVOKABLE bool drop(qreal x, qreal y, const QString& text);

signals:
    void sceneChanged();
    void zoomChanged();
    void contentXChanged();
    void contentYChanged();
    void contentSizeChanged();
    void interactiveChanged();
    void backgroundColorChanged();
    // Emitted when the scene asks for a context menu. x/y are view coordinates.
    void popupMenuRequested(QObject* menu, qreal x, qreal y);
    void clicked(qreal sceneX, qreal sceneY);
    void wheelZoomRequested(qreal factor, qreal x, qreal y);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private slots:
    void onSceneChanged();
    void onSceneRectChanged();
    void onCursorChanged();
    void onPopupMenuRequested(LimeReport::PopupMenu* menu, const QPointF& scenePos);
    void onSceneDestroyed();

private:
    QPointF toScene(const QPointF& viewPos) const;
    QPointF toGlobal(const QPointF& viewPos) const;
    void updateCursor(const QPointF& scenePos);
    void clampContentPos();

private:
    QPointer<GraphicsScene> m_scene;
    qreal m_zoom;
    qreal m_contentX;
    qreal m_contentY;
    bool m_interactive;
    QColor m_backgroundColor;
    QPointF m_lastScenePos;
};

} // namespace LimeReport

#endif // LRQUICKSCENEVIEW_H
