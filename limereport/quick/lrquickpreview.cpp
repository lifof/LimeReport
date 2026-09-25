#include "lrquickpreview.h"

#include <QFileInfo>
#include <QDir>
#include <QColor>
#include <QDesktopServices>

#include "lrreportengine_p.h"
#include "lrpagedesignintf.h"
#include "lrpreparedpages.h"
#include "lrexportersfactory.h"
#include "lrexporterintf.h"

namespace LimeReport {

QuickReportPreview::QuickReportPreview(QObject* parent)
    : QObject(parent), m_scene(nullptr), m_currentPage(0), m_busy(false), m_editMode(false)
{
}

QuickReportPreview::~QuickReportPreview()
{
    delete m_scene;
}

QObject* QuickReportPreview::engineObject() const
{
    return m_engine;
}

void QuickReportPreview::setEngineObject(QObject* engineObject)
{
    ReportEngine* engine = qobject_cast<ReportEngine*>(engineObject);
    if (m_engine == engine) return;
    if (m_engine) disconnect(m_engine, nullptr, this, nullptr);
    clear();
    m_engine = engine;
    if (m_engine) {
        connect(m_engine, &QObject::destroyed, this, &QuickReportPreview::onEngineDestroyed);
        connect(m_engine, &ReportEngine::renderPageFinished, this, &QuickReportPreview::renderPageFinished);
    }
    emit engineChanged();
}

void QuickReportPreview::onEngineDestroyed()
{
    clear();
    m_engine = nullptr;
    emit engineChanged();
}

ReportEnginePrivate* QuickReportPreview::d() const
{
    return m_engine ? m_engine->d_ptr : nullptr;
}

QObject* QuickReportPreview::scene() const
{
    return m_scene;
}

void QuickReportPreview::setCurrentPage(int page)
{
    if (m_pages.isEmpty()) page = 0;
    else page = qBound(1, page, m_pages.count());
    if (page == m_currentPage) return;
    m_currentPage = page;
    emit currentPageChanged();
}

void QuickReportPreview::setEditMode(bool value)
{
    if (m_editMode == value) return;
    m_editMode = value;
    if (m_scene) {
        if (!value) m_scene->startEditMode();
        m_scene->setItemMode(value ? ItemModes(DesignMode) : PreviewMode);
        m_scene->clearSelection();
    }
    emit editModeChanged();
}

QString QuickReportPreview::title() const
{
    return d() ? d()->previewWindowTitle() : QString();
}

QColor QuickReportPreview::pageBackgroundColor() const
{
    return d() ? d()->previewWindowPageBackground() : QColor(Qt::gray);
}

bool QuickReportPreview::resultEditable() const
{
    return d() ? d()->resultIsEditable() : false;
}

bool QuickReportPreview::printVisible() const
{
    return d() ? d()->printIsVisible() : true;
}

bool QuickReportPreview::printToPdfVisible() const
{
    return d() ? d()->printToPdfIsVisible() : true;
}

bool QuickReportPreview::saveToFileVisible() const
{
    return d() ? d()->saveToFileIsVisible() : true;
}

int QuickReportPreview::scaleType() const
{
    return d() ? int(d()->previewScaleType()) : int(FitWidth);
}

int QuickReportPreview::scalePercent() const
{
    return d() ? d()->previewScalePercent() : 100;
}

QVariantList QuickReportPreview::exporters() const
{
    QVariantList result;
    const auto map = ExportersFactory::instance().attribsMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        QVariantMap item;
        item.insert("name", it.key());
        item.insert("description", it.value().m_alias);
        result.append(item);
    }
    return result;
}

void QuickReportPreview::clear()
{
    if (m_scene) {
        PageDesignIntf* scene = m_scene;
        m_scene = nullptr;
        emit pagesChanged();
        delete scene;
    }
    m_pages.clear();
    m_errorMessages.clear();
    m_currentPage = 0;
    emit pagesChanged();
    emit currentPageChanged();
}

void QuickReportPreview::setPages(ReportPages pages)
{
    clear();
    m_pages = pages;
    if (d()) {
        m_scene = d()->createPreviewPage();
        m_errorMessages = d()->dataManager()->errorsList();
    } else {
        m_scene = new PageDesignIntf();
    }
    m_scene->setParent(nullptr);
    m_scene->setItemMode(m_editMode ? ItemModes(DesignMode) : PreviewMode);
    m_scene->setBackgroundBrush(Qt::NoBrush);
    if (!m_pages.isEmpty()) m_scene->setPageItems(m_pages);
    connect(m_scene, &GraphicsScene::selectionChanged, this, &QuickReportPreview::onSelectionChanged);
    m_currentPage = m_pages.isEmpty() ? 0 : 1;
    emit pagesChanged();
    emit currentPageChanged();
}

bool QuickReportPreview::render()
{
    if (!d() || d()->isBusy()) return false;
    m_busy = true;
    emit busyChanged();
    bool result = false;
    try {
        bool designTime = d()->dataManager()->designTime();
        d()->dataManager()->setDesignTime(false);
        ReportPages pages = d()->renderToPages();
        d()->dataManager()->setDesignTime(designTime);
        setPages(pages);
        result = !pages.isEmpty();
        if (!result) setLastError(d()->lastError());
    } catch (ReportError& error) {
        setLastError(error.what());
    }
    m_busy = false;
    emit busyChanged();
    return result;
}

QRectF QuickReportPreview::pageRect(int page) const
{
    if (page < 1 || page > m_pages.count()) return QRectF();
    PageItemDesignIntf::Ptr item = m_pages.at(page - 1);
    return item->mapRectToScene(item->rect());
}

int QuickReportPreview::pageAt(qreal sceneX, qreal sceneY) const
{
    Q_UNUSED(sceneX)
    for (int i = 0; i < m_pages.count(); ++i) {
        QRectF r = pageRect(i + 1);
        // pages are stacked vertically with a gap between them
        if (sceneY < r.bottom() + 10) return i + 1;
    }
    return m_pages.count();
}

void QuickReportPreview::onSelectionChanged()
{
    if (!m_scene || m_editMode) return;
    foreach (GraphicsItem* item, m_scene->selectedItems()) {
        for (int i = 0; i < m_pages.count(); ++i) {
            if (m_pages.at(i).data() == item) {
                setCurrentPage(i + 1);
                return;
            }
        }
    }
}

QString QuickReportPreview::localFile(const QUrl& url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

QString QuickReportPreview::defaultFileName(const QString& extension) const
{
    QString name = d() ? d()->reportName() : QString();
    if (name.isEmpty()) name = QStringLiteral("report");
    name = QFileInfo(name).completeBaseName();
    QString dir = d() && !d()->currentReportsDir().isEmpty() ? d()->currentReportsDir() : QDir::homePath();
    return QDir(dir).filePath(name + "." + extension);
}

bool QuickReportPreview::exportToPdf(const QUrl& fileUrl)
{
    if (!d() || m_pages.isEmpty()) return false;
    QString fileName = localFile(fileUrl);
    if (QFileInfo(fileName).suffix().isEmpty()) fileName += ".pdf";
    bool result = d()->printPagesToPDF(m_pages, fileName);
    if (!result) setLastError(tr("Can't write file %1").arg(fileName));
    return result;
}

bool QuickReportPreview::exportTo(const QString& exporterName, const QUrl& fileUrl)
{
    if (!d() || m_pages.isEmpty()) return false;
    if (!ExportersFactory::instance().map().contains(exporterName)) return false;
    ReportExporterInterface* exporter = ExportersFactory::instance().objectCreator(exporterName)(d());
    QString fileName = localFile(fileUrl);
    if (QFileInfo(fileName).suffix().isEmpty()) fileName += "." + exporter->exporterFileExt();
    bool result = exporter->exportPages(m_pages, fileName, QMap<QString, QVariant>());
    delete exporter;
    if (!result) setLastError(tr("Export to %1 failed").arg(fileName));
    return result;
}

bool QuickReportPreview::print()
{
    if (!d() || m_pages.isEmpty()) return false;
    // Without QtPrintSupport, printing goes through the platform PDF viewer.
    QString fileName = QDir(QDir::tempPath()).filePath(QFileInfo(defaultFileName("pdf")).fileName());
    if (!d()->printPagesToPDF(m_pages, fileName)) return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

bool QuickReportPreview::savePages(const QUrl& fileUrl)
{
    if (m_pages.isEmpty()) return false;
    bool saved = false;
    PreparedPages pagesManager(&m_pages);
    if (m_engine) emit m_engine->onSavePreview(saved, &pagesManager);
    if (saved) return true;
    IPreparedPages* manager = &pagesManager;
    return manager->saveToFile(localFile(fileUrl));
}

bool QuickReportPreview::loadPages(const QUrl& fileUrl)
{
    ReportPages pages;
    PreparedPages pagesManager(&pages);
    IPreparedPages* manager = &pagesManager;
    if (!manager->loadFromFile(localFile(fileUrl))) {
        setLastError(tr("Can't load file %1").arg(localFile(fileUrl)));
        return false;
    }
    setPages(pages);
    return true;
}

void QuickReportPreview::startInsertTextItem()
{
    if (m_scene) m_scene->startInsertMode("TextItem");
}

void QuickReportPreview::selectionMode()
{
    if (m_scene) m_scene->startEditMode();
}

void QuickReportPreview::deleteSelectedItems()
{
    if (m_scene) m_scene->deleteSelected();
}

void QuickReportPreview::setLastError(const QString& error)
{
    m_lastError = error;
    emit lastErrorChanged();
}

} // namespace LimeReport
