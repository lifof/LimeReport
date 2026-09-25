#include "lrgraphicsscene.h"

#include <QImage>
#include <QKeyEvent>
#include <QPainter>
#include <QSignalSpy>
#include <QtTest>

using namespace LimeReport;

class RectItem: public GraphicsItem {
public:
    explicit RectItem(const QRectF& r, GraphicsItem* parent = nullptr, QColor c = Qt::red):
        GraphicsItem(parent),
        rect(r),
        color(c)
    {
    }
    QRectF boundingRect() const override { return rect; }
    void paint(QPainter* painter, const StyleOptionGraphicsItem*) override
    {
        painter->fillRect(rect, color);
    }
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
    {
        changes.append(change);
        return GraphicsItem::itemChange(change, value);
    }
    void mousePressEvent(GraphicsSceneMouseEvent* event) override
    {
        ++presses;
        GraphicsItem::mousePressEvent(event);
    }
    void hoverEnterEvent(GraphicsSceneHoverEvent* event) override
    {
        ++hoverEnters;
        GraphicsItem::hoverEnterEvent(event);
    }
    void hoverLeaveEvent(GraphicsSceneHoverEvent* event) override
    {
        ++hoverLeaves;
        GraphicsItem::hoverLeaveEvent(event);
    }
    QRectF rect;
    QColor color;
    QList<int> changes;
    int presses = 0;
    int hoverEnters = 0;
    int hoverLeaves = 0;
};

class GraphicsSceneTest: public QObject {
    Q_OBJECT
private Q_SLOTS:
    void hierarchyAndMapping();
    void stackingAndHitTesting();
    void selection();
    void mouseMoveMovesSelectedItems();
    void hoverDispatch();
    void rendering();
    void clipping();
    void deletion();
};

void GraphicsSceneTest::hierarchyAndMapping()
{
    GraphicsScene scene;
    RectItem* parent = new RectItem(QRectF(0, 0, 100, 100));
    scene.addItem(parent);
    parent->setPos(10, 20);
    RectItem* child = new RectItem(QRectF(0, 0, 10, 10), parent);
    child->setPos(5, 5);
    QCOMPARE(child->scene(), &scene);
    QCOMPARE(child->parentItem(), parent);
    QCOMPARE(parent->childItems().count(), 1);
    QCOMPARE(child->scenePos(), QPointF(15, 25));
    QCOMPARE(child->mapFromScene(QPointF(15, 25)), QPointF(0, 0));
    QCOMPARE(child->sceneBoundingRect(), QRectF(15, 25, 10, 10));
    QCOMPARE(child->topLevelItem(), parent);
    QVERIFY(parent->isAncestorOf(child));

    child->setScale(2);
    QCOMPARE(child->mapToScene(QPointF(10, 10)), QPointF(35, 45));

    child->setParentItem(nullptr);
    QCOMPARE(child->scene(), &scene);
    QVERIFY(parent->childItems().isEmpty());
    QCOMPARE(scene.items().count(), 2);
    scene.removeItem(child);
    QVERIFY(!child->scene());
    delete child;
}

void GraphicsSceneTest::stackingAndHitTesting()
{
    GraphicsScene scene;
    RectItem* bottom = new RectItem(QRectF(0, 0, 100, 100));
    RectItem* top = new RectItem(QRectF(0, 0, 50, 50));
    scene.addItem(bottom);
    scene.addItem(top);
    QCOMPARE(scene.itemAt(QPointF(10, 10)), top);
    top->setZValue(-1);
    QCOMPARE(scene.itemAt(QPointF(10, 10)), bottom);
    QCOMPARE(scene.items(QPointF(10, 10)).count(), 2);
    QCOMPARE(scene.items(QPointF(80, 80)).count(), 1);
    top->setVisible(false);
    QCOMPARE(scene.items(QPointF(10, 10)).count(), 1);
    QVERIFY(bottom->collidesWithItem(top));
}

void GraphicsSceneTest::selection()
{
    GraphicsScene scene;
    RectItem* a = new RectItem(QRectF(0, 0, 10, 10));
    RectItem* b = new RectItem(QRectF(20, 0, 10, 10));
    scene.addItem(a);
    scene.addItem(b);
    QSignalSpy spy(&scene, &GraphicsScene::selectionChanged);
    a->setSelected(true);
    QVERIFY(!a->isSelected()); // not selectable yet
    a->setFlag(GraphicsItem::ItemIsSelectable);
    b->setFlag(GraphicsItem::ItemIsSelectable);
    a->setSelected(true);
    QVERIFY(a->isSelected());
    QVERIFY(a->changes.contains(GraphicsItem::ItemSelectedChange));
    QCOMPARE(spy.count(), 1);

    // click on b replaces the selection, ctrl-click extends it
    scene.handleMousePress(QPointF(25, 5), QPointF(), Qt::LeftButton, Qt::LeftButton,
                           Qt::NoModifier);
    scene.handleMouseRelease(QPointF(25, 5), QPointF(), Qt::LeftButton, Qt::NoButton,
                             Qt::NoModifier);
    QCOMPARE(scene.selectedItems(), QList<GraphicsItem*>() << b);
    scene.handleMousePress(QPointF(5, 5), QPointF(), Qt::LeftButton, Qt::LeftButton,
                           Qt::ControlModifier);
    scene.handleMouseRelease(QPointF(5, 5), QPointF(), Qt::LeftButton, Qt::NoButton,
                             Qt::ControlModifier);
    QCOMPARE(scene.selectedItems().count(), 2);

    // click on empty space clears it
    scene.handleMousePress(QPointF(500, 500), QPointF(), Qt::LeftButton, Qt::LeftButton,
                           Qt::NoModifier);
    scene.handleMouseRelease(QPointF(500, 500), QPointF(), Qt::LeftButton, Qt::NoButton,
                             Qt::NoModifier);
    QVERIFY(scene.selectedItems().isEmpty());
}

void GraphicsSceneTest::mouseMoveMovesSelectedItems()
{
    GraphicsScene scene;
    RectItem* a = new RectItem(QRectF(0, 0, 10, 10));
    RectItem* b = new RectItem(QRectF(0, 0, 10, 10));
    b->setPos(50, 0);
    scene.addItem(a);
    scene.addItem(b);
    for (RectItem* item : { a, b }) {
        item->setFlag(GraphicsItem::ItemIsSelectable);
        item->setFlag(GraphicsItem::ItemIsMovable);
        item->setSelected(true);
    }
    scene.handleMousePress(QPointF(5, 5), QPointF(), Qt::LeftButton, Qt::LeftButton,
                           Qt::NoModifier);
    QCOMPARE(a->presses, 1);
    scene.handleMouseMove(QPointF(15, 25), QPointF(), Qt::LeftButton, Qt::NoModifier);
    scene.handleMouseRelease(QPointF(15, 25), QPointF(), Qt::LeftButton, Qt::NoButton,
                             Qt::NoModifier);
    QCOMPARE(a->pos(), QPointF(10, 20));
    QCOMPARE(b->pos(), QPointF(60, 20));
}

void GraphicsSceneTest::hoverDispatch()
{
    GraphicsScene scene;
    RectItem* a = new RectItem(QRectF(0, 0, 10, 10));
    a->setAcceptHoverEvents(true);
    scene.addItem(a);
    scene.handleMouseMove(QPointF(5, 5), QPointF(), Qt::NoButton, Qt::NoModifier);
    QCOMPARE(a->hoverEnters, 1);
    scene.handleMouseMove(QPointF(50, 50), QPointF(), Qt::NoButton, Qt::NoModifier);
    QCOMPARE(a->hoverLeaves, 1);
    a->setCursor(Qt::SizeHorCursor);
    QCOMPARE(scene.cursorAt(QPointF(5, 5)).shape(), Qt::SizeHorCursor);
}

void GraphicsSceneTest::rendering()
{
    GraphicsScene scene;
    scene.setSceneRect(0, 0, 100, 100);
    scene.setBackgroundBrush(Qt::white);
    RectItem* a = new RectItem(QRectF(0, 0, 50, 50), nullptr, Qt::red);
    scene.addItem(a);
    RectItem* b = new RectItem(QRectF(0, 0, 50, 50), nullptr, Qt::blue);
    b->setPos(50, 50);
    scene.addItem(b);
    QImage image(200, 200, QImage::Format_ARGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    scene.render(&painter, QRectF(0, 0, 200, 200), QRectF(0, 0, 100, 100));
    painter.end();
    QCOMPARE(image.pixelColor(20, 20), QColor(Qt::red));
    QCOMPARE(image.pixelColor(150, 150), QColor(Qt::blue));
    QCOMPARE(image.pixelColor(150, 20), QColor(Qt::white));
}

void GraphicsSceneTest::clipping()
{
    GraphicsScene scene;
    scene.setSceneRect(0, 0, 100, 100);
    RectItem* parent = new RectItem(QRectF(0, 0, 50, 50), nullptr, Qt::white);
    parent->setFlag(GraphicsItem::ItemClipsChildrenToShape);
    scene.addItem(parent);
    RectItem* child = new RectItem(QRectF(0, 0, 100, 100), parent, Qt::green);
    Q_UNUSED(child)
    QImage image(100, 100, QImage::Format_ARGB32);
    image.fill(Qt::black);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();
    QCOMPARE(image.pixelColor(25, 25), QColor(Qt::green));
    QCOMPARE(image.pixelColor(75, 75), QColor(Qt::black));
    // hit testing respects the clip as well
    QCOMPARE(scene.items(QPointF(75, 75)).count(), 0);
}

void GraphicsSceneTest::deletion()
{
    GraphicsScene* scene = new GraphicsScene;
    RectItem* parent = new RectItem(QRectF(0, 0, 50, 50));
    scene->addItem(parent);
    RectItem* child = new RectItem(QRectF(0, 0, 10, 10), parent);
    child->setFlag(GraphicsItem::ItemIsSelectable);
    child->setSelected(true);
    QSignalSpy spy(scene, &GraphicsScene::selectionChanged);
    delete child;
    QVERIFY(parent->childItems().isEmpty());
    QCOMPARE(spy.count(), 1);
    delete scene; // deletes parent
}

QTEST_MAIN(GraphicsSceneTest)

#include "tst_graphicsscene.moc"
