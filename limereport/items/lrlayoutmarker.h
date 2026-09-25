#ifndef LRLAYOUTMARKER_H
#define LRLAYOUTMARKER_H

#include "lrbanddesignintf.h"

namespace LimeReport {

class LayoutMarker: public GraphicsItem {
public:
    explicit LayoutMarker(BaseDesignIntf* layout, GraphicsItem* parent = 0);
    virtual QRectF boundingRect() const { return m_rect; }
    virtual void paint(QPainter* painter, const StyleOptionGraphicsItem*);
    void setHeight(qreal height);
    void setWidth(qreal width);
    void setColor(QColor color);
    qreal width() { return m_rect.width(); }
    qreal height() { return m_rect.height(); }

protected:
    void mousePressEvent(GraphicsSceneMouseEvent* event);

private:
    QRectF m_rect;
    QColor m_color;
    BaseDesignIntf* m_layout;
};

} // namespace LimeReport
#endif // LRLAYOUTMARKER_H
