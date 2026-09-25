#include "lrpdfexporter.h"

#include "lrexportersfactory.h"
#include "lrreportengine_p.h"

namespace {

LimeReport::ReportExporterInterface* createPDFExporter(LimeReport::ReportEnginePrivate* parent)
{
    return new LimeReport::PDFExporter(parent);
}

bool VARIABLE_IS_NOT_USED registred = LimeReport::ExportersFactory::instance().registerCreator(
    "PDF", LimeReport::ExporterAttribs(QObject::tr("Export to PDF"), "PDFExporter"),
    createPDFExporter);

} // namespace

namespace LimeReport {

PDFExporter::PDFExporter(ReportEnginePrivate* parent): QObject(parent), m_reportEngine(parent) { }

bool PDFExporter::exportPages(ReportPages pages, const QString& fileName,
                              const QMap<QString, QVariant>& params)
{
    Q_UNUSED(params);
    if (!fileName.isEmpty()) {
        if (pages.isEmpty())
            return false;
        return m_reportEngine->printPagesToPDF(pages, fileName);
    }
    return false;
}

} // namespace LimeReport
