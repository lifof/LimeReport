#include "bands/lrdataband.h"
#include "bands/lrgroupbands.h"
#include "bands/lrpagefooter.h"
#include "bands/lrpageheader.h"
#include "bands/lrreportheader.h"
#include "bands/lrreportfooter.h"
#include "bands/lrsubdetailband.h"
#include "bands/lrtearoffband.h"


#include "items/lrtextitem.h"
#ifdef HAVE_ZINT
#include "items/lrbarcodeitem.h"
#endif
#include "items/lrhorizontallayout.h"
#include "items/lrimageitem.h"
#include "items/lrshapeitem.h"
#include "items/lrchartitem.h"
#include "lrdesignelementsfactory.h"
#ifdef HAVE_SVG
#include "items/lrsvgitem.h"
#endif



#include "serializators/lrxmlbasetypesserializators.h"
#include "serializators/lrxmlqrectserializator.h"
#include "serializators/lrxmlserializatorsfactory.h"

#include "lrexportersfactory.h"
#include "lrexporterintf.h"
#include "exporters/lrpdfexporter.h"

void initResources(){
    Q_INIT_RESOURCE(report);
    Q_INIT_RESOURCE(items);
}

namespace LimeReport{

BaseDesignIntf * createDataBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::DataBand(owner,parent);
}
BaseDesignIntf * createHeaderDataBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::DataHeaderBand(owner,parent);
}
BaseDesignIntf * createFooterDataBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::DataFooterBand(owner,parent);
}

BaseDesignIntf* createGroupHeaderBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::GroupBandHeader(owner,parent);
}

BaseDesignIntf * createGroupFooterBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::GroupBandFooter(owner,parent);
}

BaseDesignIntf * createPageHeaderBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::PageHeader(owner,parent);
}

BaseDesignIntf * createPageFooterBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::PageFooter(owner,parent);
}

BaseDesignIntf * createSubDetailBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::SubDetailBand(owner,parent);
}

BaseDesignIntf * createSubDetailHeaderBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::SubDetailHeaderBand(owner,parent);
}

BaseDesignIntf * createSubDetailFooterBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::SubDetailFooterBand(owner,parent);
}

BaseDesignIntf * createTearOffBand(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::TearOffBand(owner,parent);
}

BaseDesignIntf * createTextItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::TextItem(owner,parent);
}

#ifdef HAVE_ZINT
BaseDesignIntf * createBarcodeItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new BarcodeItem(owner,parent);
}
#endif

#ifdef HAVE_SVG
BaseDesignIntf* createSVGItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new SVGItem(owner,parent);
}
#endif

BaseDesignIntf* createHLayout(QObject *owner, LimeReport::BaseDesignIntf  *parent)
{
    return new HorizontalLayout(owner, parent);
}

BaseDesignIntf* createImageItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new ImageItem(owner,parent);
}

BaseDesignIntf* createShapeItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new ShapeItem(owner,parent);
}

BaseDesignIntf* createChartItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new ChartItem(owner,parent);
}

void initReportItems(){
    initResources();
    DesignElementsFactory::instance().registerCreator(
                "TextItem",
                LimeReport::ItemAttribs(QObject::tr("Text Item"),"TextItem"),
                createTextItem
    );
#ifdef HAVE_ZINT
    DesignElementsFactory::instance().registerCreator(
                "BarcodeItem",
                LimeReport::ItemAttribs(QObject::tr("Barcode Item"),"Item"),
                createBarcodeItem
    );
#endif

    DesignElementsFactory::instance().registerCreator(
                "HLayout",
                LimeReport::ItemAttribs(QObject::tr("HLayout"), LimeReport::Const::bandTAG),
                createHLayout
    );
    DesignElementsFactory::instance().registerCreator(
                         "ImageItem", LimeReport::ItemAttribs(QObject::tr("Image Item"),"Item"), createImageItem
    );

#ifdef HAVE_SVG
    DesignElementsFactory::instance().registerCreator(
        "BarcodeItem",
        LimeReport::ItemAttribs(QObject::tr("SVG Item"),"Item"),
        createSVGItem
        );
#endif

    DesignElementsFactory::instance().registerCreator(
                         "ShapeItem", LimeReport::ItemAttribs(QObject::tr("Shape Item"),"Item"), createShapeItem
    );
    DesignElementsFactory::instance().registerCreator(
                         "ChartItem", LimeReport::ItemAttribs(QObject::tr("Chart Item"),"Item"), createChartItem
    );
    DesignElementsFactory::instance().registerCreator(
            "Data",
            LimeReport::ItemAttribs(QObject::tr("Data"),LimeReport::Const::bandTAG),
            createDataBand
    );
    DesignElementsFactory::instance().registerCreator(
                "DataHeader",
                LimeReport::ItemAttribs(QObject::tr("DataHeader"),LimeReport::Const::bandTAG),
                createHeaderDataBand
    );
    DesignElementsFactory::instance().registerCreator(
                "DataFooter",
                LimeReport::ItemAttribs(QObject::tr("DataFooter"),LimeReport::Const::bandTAG),
                createFooterDataBand
    );
    DesignElementsFactory::instance().registerCreator(
           "GroupHeader",
            LimeReport::ItemAttribs(QObject::tr("GroupHeader"),LimeReport::Const::bandTAG),
            createGroupHeaderBand
    );
    DesignElementsFactory::instance().registerCreator(
            "GroupFooter",
            LimeReport::ItemAttribs(QObject::tr("GroupFooter"),LimeReport::Const::bandTAG),
            createGroupFooterBand
    );
    DesignElementsFactory::instance().registerCreator(
            "PageFooter",
            LimeReport::ItemAttribs(QObject::tr("Page Footer"),LimeReport::Const::bandTAG),
            createPageFooterBand
    );
    DesignElementsFactory::instance().registerCreator(
            "PageHeader",
            LimeReport::ItemAttribs(QObject::tr("Page Header"),LimeReport::Const::bandTAG),
            createPageHeaderBand
    );
    DesignElementsFactory::instance().registerCreator(
            "SubDetail",
            LimeReport::ItemAttribs(QObject::tr("SubDetail"),LimeReport::Const::bandTAG),
            createSubDetailBand
    );

    DesignElementsFactory::instance().registerCreator(
           "SubDetailHeader",
            LimeReport::ItemAttribs(QObject::tr("SubDetailHeader"),LimeReport::Const::bandTAG),
            createSubDetailHeaderBand
    );
    DesignElementsFactory::instance().registerCreator(
            "SubDetailFooter",
            LimeReport::ItemAttribs(QObject::tr("SubDetailFooter"),LimeReport::Const::bandTAG),
            createSubDetailFooterBand
    );
    DesignElementsFactory::instance().registerCreator(
            "TearOffBand",
            LimeReport::ItemAttribs(QObject::tr("Tear-off Band"),LimeReport::Const::bandTAG),
            createTearOffBand
    );

}


SerializatorIntf * createIntSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlIntSerializator(doc,node);
}

SerializatorIntf * createQRealSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlQRealSerializator(doc,node);
}

SerializatorIntf * createQStringSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlQStringSerializator(doc,node);
}

SerializatorIntf * createEnumAndFlagsSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlEnumAndFlagsSerializator(doc,node);
}

SerializatorIntf * createBoolSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlBoolSerializator(doc,node);
}

SerializatorIntf * createFontSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlFontSerializator(doc,node);
}

SerializatorIntf * createQSizeFSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlQSizeFSerializator(doc,node);
}

SerializatorIntf * createQImageSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlQImageSerializator(doc,node);
}

SerializatorIntf * createQColorSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlColorSerializator(doc,node);
}

SerializatorIntf* createQByteArraySerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlQByteArraySerializator(doc,node);
}

SerializatorIntf* createQVariantSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XmlQVariantSerializator(doc,node);
}

SerializatorIntf * createQRectSerializator(QDomDocument *doc, QDomElement *node){
    return new LimeReport::XMLQRectSerializator(doc,node);
}

void initSerializators()
{
    XMLAbstractSerializatorFactory::instance().registerCreator("QString", createQStringSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("int", createIntSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("enumAndFlags",createEnumAndFlagsSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("bool", createBoolSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QFont", createFontSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QSizeF", createQSizeFSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QImage", createQImageSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("qreal", createQRealSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("double", createQRealSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QColor", createQColorSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QByteArray", createQByteArraySerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QVariant", createQVariantSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QRect", createQRectSerializator);
    XMLAbstractSerializatorFactory::instance().registerCreator("QRectF", createQRectSerializator);
}

LimeReport::ReportExporterInterface* createPDFExporter(ReportEnginePrivate* parent){
    return new LimeReport::PDFExporter(parent);
}

void initExporters()
{
    ExportersFactory::instance().registerCreator(
                "PDF",
                LimeReport::ExporterAttribs(QObject::tr("Export to PDF"), "PDFExporter"),
                createPDFExporter
    );
}

} //namespace LimeReport
