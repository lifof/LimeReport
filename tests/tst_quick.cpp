#include <QtTest>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>

#include "lrreportengine.h"
#include "lrreportengine_p.h"
#include "lrpagedesignintf.h"
#include "lrbanddesignintf.h"
#include "quick/lrquickdesigner.h"
#include "quick/lrquickpreview.h"
#include "quick/lrquickpropertymodel.h"
#include "quick/lrquickdatamodel.h"

using namespace LimeReport;

class QuickTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void renderDemoReports_data();
    void renderDemoReports();
    void designerEditing();
    void propertyModel();
    void dataBrowser();
    void saveAndReload();
    void previewController();
    void qmlComponents_data();
    void qmlComponents();
private:
    QTemporaryDir m_dir;
};

void QuickTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
    QDir::setCurrent(QStringLiteral(DEMO_DIR));
}

void QuickTest::renderDemoReports_data()
{
    QTest::addColumn<QString>("report");
    QTest::addColumn<int>("minPages");
    QTest::newRow("simple_list") << "simple_list.lrxml" << 2;
    QTest::newRow("group_subdetail") << "demoReport1_report_header_group_subdetail.lrxml" << 2;
    QTest::newRow("toc") << "demoReport1_report_header_group_subdetail_TOC.lrxml" << 2;
    QTest::newRow("categories") << "categories.lrxml" << 1;
}

void QuickTest::renderDemoReports()
{
    QFETCH(QString, report);
    QFETCH(int, minPages);
    ReportEngine engine;
    QVERIFY2(engine.loadFromFile("demo_reports/" + report), qPrintable(engine.lastError()));
    QList<QImage> images = engine.renderToImages(50);
    QVERIFY2(images.count() >= minPages, qPrintable(QString::number(images.count())));
    QVERIFY(!images.first().isNull());
    // something besides the white page background has been painted
    QImage first = images.first().convertToFormat(QImage::Format_RGB32);
    int dark = 0;
    for (int y = 0; y < first.height(); y += 2)
        for (int x = 0; x < first.width(); x += 2)
            if (qGray(first.pixel(x, y)) < 128) ++dark;
    QVERIFY(dark > 100);

    QString pdf = m_dir.filePath(report + ".pdf");
    QVERIFY(engine.printToPDF(pdf));
    QFile file(pdf);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.read(4) == "%PDF");
    QVERIFY(file.size() > 2000);
}

static int countItems(PageDesignIntf* page, const char* className)
{
    int result = 0;
    for (BaseDesignIntf* item : page->pageItem()->allChildBaseItems())
        if (item->inherits(className)) ++result;
    return result;
}

void QuickTest::designerEditing()
{
    ReportEngine engine;
    QuickReportDesigner designer;
    designer.setEngineObject(&engine);
    QCOMPARE(designer.pageNames().count(), 1);
    PageDesignIntf* page = designer.currentPage();
    QVERIFY(page);

    designer.addBand(BandDesignIntf::Data);
    QCOMPARE(countItems(page, "LimeReport::BandDesignIntf"), 1);
    BandDesignIntf* band = page->pageItem()->bands().first();
    QVERIFY(designer.selectedObject() == band);

    // insert a text item by clicking into the band
    designer.startInsert("TextItem");
    QCOMPARE(designer.insertItemType(), QString("TextItem"));
    QPointF pos = band->mapToScene(QPointF(50, 10));
    page->handleMousePress(pos, QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    page->handleMouseRelease(pos, QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QVERIFY(designer.insertItemType().isEmpty());
    QCOMPARE(countItems(page, "LimeReport::TextItem"), 1);
    QVERIFY(designer.canUndo());
    QVERIFY(designer.modified());

    designer.undo();
    QCOMPARE(countItems(page, "LimeReport::TextItem"), 0);
    designer.redo();
    QCOMPARE(countItems(page, "LimeReport::TextItem"), 1);

    // clipboard round trip
    BaseDesignIntf* text = page->reportItemsByName("TextItem1").value(0);
    QVERIFY(text);
    designer.selectObject(text);
    designer.copy();
    designer.selectObject(band);
    designer.paste();
    QCOMPARE(countItems(page, "LimeReport::TextItem"), 2);
    designer.deleteSelected();
    QVERIFY(countItems(page, "LimeReport::TextItem") <= 2);

    designer.addPage();
    QCOMPARE(designer.pageNames().count(), 2);
    QCOMPARE(designer.currentPageIndex(), 1);
    QVERIFY(designer.deletePage(1));
    QCOMPARE(designer.pageNames().count(), 1);
}

void QuickTest::propertyModel()
{
    ReportEngine engine;
    QuickReportDesigner designer;
    designer.setEngineObject(&engine);
    PageDesignIntf* page = designer.currentPage();
    designer.addBand(BandDesignIntf::Data);
    BandDesignIntf* band = page->pageItem()->bands().first();
    BaseDesignIntf* text = page->addReportItem("TextItem", band->mapToScene(QPointF(10, 10)), QSizeF(300, 50));
    QVERIFY(text);
    designer.selectObject(text);

    QuickPropertyModel* model = qobject_cast<QuickPropertyModel*>(designer.propertyModel());
    QVERIFY(model);
    QCOMPARE(model->objectCount(), 1);
    int contentRow = model->rowOf("content");
    QVERIFY(contentRow >= 0);
    QCOMPARE(model->data(model->index(contentRow), QuickPropertyModel::EditorRole).toString(), QString("text"));
    QVERIFY(model->setValue(contentRow, "Hello"));
    QCOMPARE(text->property("content").toString(), QString("Hello"));
    designer.undo();
    QVERIFY(text->property("content").toString() != "Hello");

    int geometryRow = model->rowOf("geometry");
    QVERIFY(geometryRow >= 0);
    QVERIFY(model->setRectValue(geometryRow, 10, 5, 40, 12));
    QCOMPARE(text->geometry().width(), 400);

    int fontRow = model->rowOf("font");
    QVERIFY(model->setFontValue(fontRow, "Arial", 14, true, false, false, false));
    QVERIFY(text->property("font").value<QFont>().bold());

    int enumRow = model->rowOf("backgroundMode");
    QCOMPARE(model->data(model->index(enumRow), QuickPropertyModel::EditorRole).toString(), QString("enum"));
    QVERIFY(model->setValue(enumRow, 0));
    QCOMPARE(text->property("backgroundMode").toInt(), 0);

    int bordersRow = model->rowOf("borders");
    QCOMPARE(model->data(model->index(bordersRow), QuickPropertyModel::EditorRole).toString(), QString("flags"));
    QVERIFY(model->setFlag(bordersRow, 1, true));
    QVERIFY(text->property("borders").toInt() & 1);
}

void QuickTest::dataBrowser()
{
    ReportEngine engine;
    QuickReportDesigner designer;
    designer.setEngineObject(&engine);
    QVariantMap connection;
    connection["name"] = "northwind";
    connection["driver"] = "QSQLITE";
    connection["databaseName"] = QString(DEMO_DIR) + "/demo_reports/northwind.db";
    connection["autoconnect"] = true;
    QVERIFY2(designer.saveConnection(connection), qPrintable(designer.lastError()));
    QVariantMap query;
    query["name"] = "customers";
    query["type"] = "query";
    query["connection"] = "northwind";
    query["sql"] = "select * from customers";
    QVERIFY2(designer.saveDatasource(query), qPrintable(designer.lastError()));
    QVERIFY(designer.datasourceNames().contains("customers"));
    QVERIFY(designer.fieldNames("customers").contains("CompanyName"));

    QVariantMap variable;
    variable["name"] = "title";
    variable["type"] = "String";
    variable["value"] = "Customers";
    QVERIFY2(designer.saveVariable(variable), qPrintable(designer.lastError()));
    QCOMPARE(designer.variableInfo("title").value("value").toString(), QString("Customers"));

    QuickDataBrowserModel* model = qobject_cast<QuickDataBrowserModel*>(designer.dataModel());
    bool fieldFound = false;
    for (int i = 0; i < model->rowCount(); ++i) {
        QVariantMap row = model->get(i);
        if (row.value("dragText").toString() == "field:$D{customers.CompanyName}") fieldFound = true;
    }
    QVERIFY(fieldFound);

    QVERIFY(designer.deleteDatasource("customers"));
    QVERIFY(!designer.datasourceNames().contains("customers"));
    QVERIFY(designer.deleteVariable("title"));
    QVERIFY(designer.deleteConnection("northwind"));
}

void QuickTest::saveAndReload()
{
    ReportEngine engine;
    QuickReportDesigner designer;
    designer.setEngineObject(&engine);
    designer.addBand(BandDesignIntf::ReportHeader);
    designer.addPage();
    designer.setScript("var x = 1;");
    QString fileName = m_dir.filePath("roundtrip.lrxml");
    QVERIFY(designer.saveReportAs(QUrl::fromLocalFile(fileName)));
    QVERIFY(!designer.modified());

    ReportEngine other;
    QuickReportDesigner otherDesigner;
    otherDesigner.setEngineObject(&other);
    QVERIFY(otherDesigner.openReport(QUrl::fromLocalFile(fileName)));
    QCOMPARE(otherDesigner.pageNames().count(), 2);
    QCOMPARE(otherDesigner.script(), QString("var x = 1;"));
    QCOMPARE(otherDesigner.currentPage()->pageItem()->bands().count(), 1);
}

void QuickTest::previewController()
{
    ReportEngine engine;
    QVERIFY(engine.loadFromFile("demo_reports/simple_list.lrxml"));
    QuickReportPreview preview;
    preview.setEngineObject(&engine);
    QVERIFY(preview.render());
    QVERIFY(preview.pageCount() >= 2);
    QCOMPARE(preview.currentPage(), 1);
    QRectF second = preview.pageRect(2);
    QVERIFY(second.top() > preview.pageRect(1).bottom());
    QCOMPARE(preview.pageAt(second.center().x(), second.center().y()), 2);
    preview.setCurrentPage(100);
    QCOMPARE(preview.currentPage(), preview.pageCount());
    QString pdf = m_dir.filePath("preview.pdf");
    QVERIFY(preview.exportToPdf(QUrl::fromLocalFile(pdf)));
    QVERIFY(QFileInfo(pdf).size() > 2000);
    QString pages = m_dir.filePath("preview.lrpx");
    QVERIFY(preview.savePages(QUrl::fromLocalFile(pages)));
    QuickReportPreview loaded;
    QVERIFY(loaded.loadPages(QUrl::fromLocalFile(pages)));
    QCOMPARE(loaded.pageCount(), preview.pageCount());
}

void QuickTest::qmlComponents_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("designer") << "import QtQuick\nimport LimeReport\nItem { width: 1200; height: 800\n"
                                 "  ReportEngine { id: engine }\n"
                                 "  ReportDesigner { anchors.fill: parent; engine: engine } }";
    QTest::newRow("preview") << "import QtQuick\nimport LimeReport\nItem { width: 800; height: 600\n"
                                "  ReportEngine { id: engine }\n"
                                "  ReportPreview { anchors.fill: parent; controller: ReportPreviewController { engine: engine } } }";
}

void QuickTest::qmlComponents()
{
    QFETCH(QString, source);
    QQmlEngine engine;
    engine.addImportPath("qrc:/qt/qml");
    QQmlComponent component(&engine);
    component.setData(source.toUtf8(), QUrl("qrc:/test.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
}

QTEST_MAIN(QuickTest)

#include "tst_quick.moc"
