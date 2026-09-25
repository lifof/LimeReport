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
#include <QGuiApplication>
#include <QFileSystemWatcher>
#include <QPdfWriter>
#include <QPainter>
#include <QDesktopServices>
#include <QTemporaryDir>
#include <QUrl>
#include <QDir>

#include "lrmessagehub.h"
#include "time.h"

#include "lrreportengine_p.h"
#include "lrreportengine.h"

#include "lrpagedesignintf.h"
#include "lrdatasourcemanager.h"


#include "serializators/lrxmlwriter.h"
#include "serializators/lrxmlreader.h"
#include "lrreportrender.h"
#include "quick/lrquickwindows.h"
#include "lrexporterintf.h"
#include "lrexportersfactory.h"

#ifdef BUILD_WITH_EASY_PROFILER
#include "easy/profiler.h"
#else
# define EASY_BLOCK(...)
# define EASY_END_BLOCK
#endif
#include "lrpreparedpages.h"

#ifdef HAVE_STATIC_BUILD
#include "lrfactoryinitializer.h"
#endif

namespace LimeReport{

QSettings* ReportEngine::m_settings = 0;

ReportEnginePrivate::ReportEnginePrivate(QObject *parent) :
    QObject(parent), m_preparedPagesManager(new PreparedPages(&m_preparedPages)), m_fileName(""), m_settings(0), m_ownedSettings(false),
    m_showProgressDialog(true), m_reportName(""), m_activePreview(0),
    m_previewWindowIcon(":/report/images/logo32"), m_previewWindowTitle(tr("Preview")),
    m_reportRendering(false), m_resultIsEditable(true), m_passPhrase("HjccbzHjlbyfCkjy"),
    m_fileWatcher( new QFileSystemWatcher( this ) ), m_reportLanguage(QLocale::AnyLanguage),
    m_previewLayoutDirection(Qt::LayoutDirectionAuto),
    m_previewScaleType(FitWidth), m_previewScalePercent(0), m_startTOCPage(0),
    m_previewPageBackgroundColor(Qt::gray),
    m_saveToFileVisible(true), m_printToPdfVisible(true),
    m_printVisible(true)
{
#ifdef HAVE_STATIC_BUILD
    initResources();
    initReportItems();
    initSerializators();
#endif
    m_datasources = new DataSourceManager(this);
    m_datasources->setReportSettings(&m_reportSettings);
    scriptManager()->setDataManager(m_datasources);
    m_scriptEngineContext = new ScriptEngineContext(this);

    ICallbackDatasource* tableOfContents = m_datasources->createCallbackDatasource("tableofcontents");
    connect(tableOfContents, SIGNAL(getCallbackData(LimeReport::CallbackInfo,QVariant&)),
            m_scriptEngineContext->tableOfContents(), SLOT(slotOneSlotDS(LimeReport::CallbackInfo,QVariant&)));

    m_datasources->setObjectName("datasources");
    connect(m_datasources,SIGNAL(loadCollectionFinished(QString)),this,SLOT(slotDataSourceCollectionLoaded(QString)));
    connect(m_fileWatcher,SIGNAL(fileChanged(const QString &)),this,SLOT(slotLoadFromFile(const QString &)));

}

ReportEnginePrivate::~ReportEnginePrivate()
{
    if (m_designerWindow) {
        QuickWindows::closeWindow(m_designerWindow);
    }
    if (m_activePreview){
        QuickWindows::closeWindow(m_activePreview);
    }
    foreach(PageDesignIntf* page,m_pages) delete page;
    m_pages.clear();

    foreach(ReportTranslation* translation, m_translations)
        delete translation;
    m_translations.clear();

    if (m_ownedSettings&&m_settings) delete m_settings;
    delete m_preparedPagesManager;
}

QObject* ReportEnginePrivate::createElement(const QString &, const QString &)
{
    return appendPage();
}

QObject *ReportEnginePrivate::elementAt(const QString &, int index)
{
    return pageAt(index);
}

PageDesignIntf *ReportEnginePrivate::createPage(const QString &pageName, bool preview)
{
    PageDesignIntf* page =new PageDesignIntf();
    page->setObjectName(pageName);
    page->pageItem()->setObjectName("Report"+pageName);
    if (!preview)
        page->setReportEditor(this);
    page->setReportSettings(&m_reportSettings);
    return page;
}

PageDesignIntf *ReportEnginePrivate::appendPage(const QString &pageName)
{
    PageDesignIntf* page = createPage(pageName);
    m_pages.append(page);
    return page;
}

bool ReportEnginePrivate::deletePage(PageDesignIntf *page){
    QList<PageDesignIntf*>::iterator it = m_pages.begin();
    while (it != m_pages.end()){
        if (*it == page) {
            it = m_pages.erase(it);
            return true;
        } else ++it;
    }
    return false;
}

PageDesignIntf *ReportEnginePrivate::createPreviewPage()
{
    return createPage("preview",true);
}

int ReportEnginePrivate::elementsCount(const QString &)
{
    return m_pages.count();
}

void ReportEnginePrivate::collectionLoadFinished(const QString &)
{
    foreach (PageDesignIntf* page, m_pages) {
        page->setReportEditor(this);
        page->setReportSettings(&m_reportSettings);
        page->setSceneRect(-Const::SCENE_MARGIN,-Const::SCENE_MARGIN,
                           page->pageItem()->width()+Const::SCENE_MARGIN*2,
                           page->pageItem()->boundingRect().height()+Const::SCENE_MARGIN*2);
    }
    emit pagesLoadFinished();
}

void ReportEnginePrivate::saveError(QString message)
{
    m_lastError = message;
}

void ReportEnginePrivate::showError(QString message)
{
    MessageHub::critical(0,tr("Error"),message);
}

void ReportEnginePrivate::updateTranslations()
{
    foreach(ReportTranslation* translation, m_translations.values()){
        translation->invalidatePages();
        foreach(PageDesignIntf* page, m_pages){
            translation->updatePageTranslation(page);
        }
        translation->clearInvalidPages();
    }
}

void ReportEnginePrivate::slotDataSourceCollectionLoaded(const QString &collectionName)
{
    emit datasourceCollectionLoadFinished(collectionName);
}

void ReportEnginePrivate::slotPreviewWindowDestroyed(QObject* window)
{
    if (m_activePreview == window){
        m_activePreview = 0;
    }
}

void ReportEnginePrivate::slotDesignerWindowDestroyed(QObject *window)
{
    Q_UNUSED(window)
    dataManager()->setDesignTime(false);
}

void ReportEnginePrivate::clearReport()
{
    foreach(PageDesignIntf* page,m_pages) delete page;
    m_pages.clear();
    foreach(ReportTranslation* reportTranslation, m_translations)
        delete reportTranslation;
    m_translations.clear();
    m_datasources->clear(DataSourceManager::Owned);
    m_fileName="";
    m_scriptEngineContext->clear();
    m_reportSettings.setDefaultValues();
    emit cleared();
}

bool ReportEnginePrivate::printPagesToPDF(ReportPages pages, const QString& fileName, const PrintRange& range)
{
    if (fileName.isEmpty()) return false;
    m_cancelPrinting = false;
    QPdfWriter writer(fileName);
    writer.setCreator(QStringLiteral("LimeReport"));
    writer.setTitle(reportName());
    writer.setResolution(300);
    PdfPrintProcessor processor(&writer);

    int pageCount = 0;
    for (int i = 0; i < pages.size(); ++i)
        if (range.contains(i + 1)) ++pageCount;

    emit printingStarted(pageCount);
    bool result = true;
    try {
        for (int i = 0; i < pages.size(); ++i){
            if (m_cancelPrinting) break;
            if (!range.contains(i + 1)) continue;
            if (!processor.printPage(pages.at(i))) { result = false; break; }
            emit pagePrintingFinished(i + 1);
            QCoreApplication::processEvents();
        }
    } catch (ReportError &exception){
        saveError(exception.what());
        result = false;
    }
    processor.finish();
    emit printingFinished();
    if (result) emitPrintedToPDF(fileName);
    return result;
}

QStringList ReportEnginePrivate::aviableReportTranslations()
{
    QStringList result;
    foreach (QLocale::Language language, aviableLanguages()){
        result << QLocale::languageToString(language);
    }
    return result;
}

void ReportEnginePrivate::setReportTranslation(const QString &languageName)
{
    foreach(QLocale::Language language, aviableLanguages()){
       if (QLocale::languageToString(language).compare(languageName) == 0){
           setReportLanguage(language);
       }
    }
}

bool ReportEnginePrivate::printReport()
{
    // Without QtPrintSupport there is no printer dialog: the report is
    // rendered to a PDF which is handed to the platform (viewer/print service).
    static QTemporaryDir tempDir;
    if (!tempDir.isValid()) return false;
    QString baseName = reportName().isEmpty() ? QStringLiteral("report") : QFileInfo(reportName()).baseName();
    QString fileName = QDir(tempDir.path()).filePath(baseName + QStringLiteral(".pdf"));
    if (!printToPDF(fileName)) return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

void ReportEnginePrivate::printToFile(const QString &fileName)
{
    if (!fileName.isEmpty()){
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)){
            QTextStream out(&file);
            try {
                dataManager()->setDesignTime(false);
                out<<renderToString();
                dataManager()->setDesignTime(true);
            }  catch( ReportError &exception){
                saveError(exception.what());
            }
        }
        file.close();
    }
}

bool ReportEnginePrivate::printToPDF(const QString &fileName, const PrintRange& range)
{
    if (fileName.isEmpty()) return false;
    bool designTime = dataManager()->designTime();
    dataManager()->setDesignTime(false);
    ReportPages pages = renderToPages();
    dataManager()->setDesignTime(designTime);
    if (pages.isEmpty()) return false;
    return printPagesToPDF(pages, fileName, range);
}

QList<QImage> ReportEnginePrivate::renderToImages(qreal dpi)
{
    QList<QImage> result;
    bool designTime = dataManager()->designTime();
    dataManager()->setDesignTime(false);
    ReportPages pages = renderToPages();
    dataManager()->setDesignTime(designTime);
    foreach (PageItemDesignIntf::Ptr page, pages) {
        // scene units are 1/10 mm
        QSizeF sizeMM = page->sizeMM();
        QSize size(qRound(sizeMM.width() / 25.4 * dpi), qRound(sizeMM.height() / 25.4 * dpi));
        QImage image(size, QImage::Format_ARGB32_Premultiplied);
        image.setDotsPerMeterX(qRound(dpi / 0.0254));
        image.setDotsPerMeterY(qRound(dpi / 0.0254));
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        PdfPrintProcessor::renderPage(page, &painter, QRectF(QPointF(0, 0), size));
        painter.end();
        result.append(image);
    }
    return result;
}

bool ReportEnginePrivate::exportReport(QString exporterName, const QString &fileName, const QMap<QString, QVariant> &params)
{
    QString fn = fileName;
    if (ExportersFactory::instance().map().contains(exporterName)){
        ReportExporterInterface* e = ExportersFactory::instance().objectCreator(exporterName)(this);
        if (fn.isEmpty()){
            QString defaultFileName = reportName().split(".")[0];
            QString filter = QString("%1 (*.%2)").arg(e->exporterName()).arg(e->exporterFileExt());
            fn = QDir(currentReportsDir().isEmpty() ? QDir::currentPath() : currentReportsDir())
                    .filePath(defaultFileName + "." + e->exporterFileExt());
            Q_UNUSED(filter)
        }
        if (!fn.isEmpty()){
            QFileInfo fi(fn);
            if (fi.suffix().isEmpty())
                fn += QString(".%1").arg(e->exporterFileExt());

            bool designTime = dataManager()->designTime();
            dataManager()->setDesignTime(false);
            ReportPages pages = renderToPages();
            dataManager()->setDesignTime(designTime);
            bool result = e->exportPages(pages, fn, params);
            delete e;
            return result;
        }
    }
    return false;
}

bool ReportEnginePrivate::showPreviewWindow(ReportPages pages, PreviewHints hints, bool modal)
{
    if (pages.count()>0){
        Q_Q(ReportEngine);
        QObject* window = QuickWindows::showPreview(q, pages, hints, modal, [this](QObject* w){
            m_activePreview = w;
            connect(w, SIGNAL(destroyed(QObject*)), this, SLOT(slotPreviewWindowDestroyed(QObject*)));
        });
        return window != 0 || modal;
    }
    return false;
}

void ReportEnginePrivate::previewReport(PreviewHints hints, bool modal)
{
    try{
        dataManager()->setDesignTime(false);
        ReportPages pages = renderToPages();
        dataManager()->setDesignTime(true);
        showPreviewWindow(pages, hints, modal);
    } catch (ReportError &exception){
        saveError(exception.what());
        showError(exception.what());
    }
}

PageDesignIntf* ReportEnginePrivate::createPreviewScene(QObject* parent){
    PageDesignIntf* result = 0;
    try {
        ReportPages pages = renderToPages();
        result = new PageDesignIntf(parent);
        result->setPageItems(pages);
        result->setItemMode(PrintMode);
    } catch (ReportError &exception){
        saveError(exception.what());
        showError(exception.what());
    }
    return result;
}

bool ReportEnginePrivate::emitSaveReport()
{
    bool result = false;
    emit onSave(result);
    return result;
}

bool ReportEnginePrivate::emitSaveReportAs()
{
    bool result = false;
    emit onSaveAs(result);
    return result;
}

bool ReportEnginePrivate::emitLoadReport()
{
    bool result = false;
    emit onLoad(result);
    return result;
}

void ReportEnginePrivate::emitSaveFinished()
{
    emit saveFinished();
}

void ReportEnginePrivate::emitLoadFinished()
{
    emit loadFinished();
}

void ReportEnginePrivate::emitPrintedToPDF(QString fileName)
{
    emit printedToPDF(fileName);
}

bool ReportEnginePrivate::isSaved()
{
    foreach (PageDesignIntf* page, m_pages) {
        if (!page->isSaved()) return false;
    }
    return true;
}

void ReportEnginePrivate::setCurrentReportsDir(const QString &dirName)
{
    if (QDir(dirName).exists())
        m_reportsDir = dirName;
}

bool ReportEnginePrivate::slotLoadFromFile(const QString &fileName)
{
    EASY_BLOCK("ReportEnginePrivate::slotLoadFromFile")
    QPointer<QObject> currentPreview = m_activePreview;
   
    if (!QFile::exists(fileName))
    {
       if ( hasActivePreview() )
       {          
          MessageHub::information( NULL,
                                    tr( "Report File Change" ),
                                    tr( "The report file \"%1\" has changed names or been deleted.\n\nThis preview is no longer valid." ).arg( fileName )
                                    );
          
          clearReport();
          
          QuickWindows::closeWindow(currentPreview);
       }
       
       return false;
    }

    clearReport();

    ItemsReaderIntf::Ptr reader = FileXMLReader::create(fileName);
    reader->setPassPhrase(m_passPhrase);
    if (reader->first()){
        if (reader->readItem(this)){
            m_fileName=fileName;
            QFileInfo fi(fileName);
            m_reportName = fi.fileName();

            QString dbSettingFileName = fi.absolutePath()+"/"+fi.baseName()+".db";
            if (QFile::exists(dbSettingFileName)){
                QSettings dbcredentals(dbSettingFileName, QSettings::IniFormat);
                foreach (ConnectionDesc* connection, dataManager()->conections()) {
                    if (!connection->keepDBCredentials()){
                        dbcredentals.beginGroup(connection->name());
                        connection->setUserName(dbcredentals.value("user").toString());
                        connection->setPassword(dbcredentals.value("password").toString());
                        dbcredentals.endGroup();
                    }
                }
            }
            EASY_BLOCK("Connect auto connections")
            dataManager()->connectAutoConnections();
            EASY_END_BLOCK;
            dropChanges();

            if ( hasActivePreview() )
            {
               QMetaObject::invokeMethod(currentPreview, "reloadPreview");
            }
            EASY_END_BLOCK;
            return true;
        }
    }
    m_lastError = reader->lastError();
    EASY_END_BLOCK;
    return false;
}

void ReportEnginePrivate::cancelRender()
{
    if (m_reportRender)
        m_reportRender->cancelRender();
    m_reportRendering = false;
}

void ReportEnginePrivate::cancelPrinting()
{
    m_cancelPrinting = true;
}

GraphicsScene* ReportEngine::createPreviewScene(QObject* parent){
    Q_D(ReportEngine);
    return d->createPreviewScene(parent);
}

void ReportEnginePrivate::designReport(bool showModal)
{
    Q_Q(ReportEngine);
    if (m_designerWindow) {
        QMetaObject::invokeMethod(m_designerWindow, "raise");
        QMetaObject::invokeMethod(m_designerWindow, "requestActivate");
        return;
    }
    dataManager()->setDesignTime(true);
    m_datasources->updateDatasourceModel();
    QuickWindows::showDesigner(q, showModal, [this](QObject* w){
        m_designerWindow = w;
        connect(w, SIGNAL(destroyed(QObject*)), this, SLOT(slotDesignerWindowDestroyed(QObject*)));
    });
}

void ReportEnginePrivate::setSettings(QSettings* value)
{
    if (value){
        if (m_ownedSettings&&m_settings)
            delete m_settings;
        m_settings = value;
        m_ownedSettings = false;
    }
}

QSettings*ReportEnginePrivate::settings()
{
    if (m_settings){
        return m_settings;
    } else {
        m_settings = new QSettings("LimeReport",QCoreApplication::applicationName());
        m_ownedSettings=true;
        return m_settings;
    }
}

bool ReportEnginePrivate::loadFromFile(const QString &fileName, bool autoLoadPreviewOnChange)
{
   // only watch one file at a time

   if ( !m_fileWatcher->files().isEmpty() )
   {
      m_fileWatcher->removePaths( m_fileWatcher->files() );
   }

   if ( autoLoadPreviewOnChange )
   {
      m_fileWatcher->addPath( fileName );
   }

   bool result = slotLoadFromFile( fileName );
   if (result) {
       emit loadFinished();
   }
   return result;
}

bool ReportEnginePrivate::loadFromByteArray(QByteArray* data, const QString &name){
    clearReport();

    ItemsReaderIntf::Ptr reader = ByteArrayXMLReader::create(data);
    reader->setPassPhrase(m_passPhrase);
    if (reader->first()){
        if (reader->readItem(this)){
            m_fileName = "";
            m_reportName = name;
            emit loadFinished();
            return true;
        };
    }
    return false;
}

bool ReportEnginePrivate::loadFromString(const QString &report, const QString &name)
{
    clearReport();

    ItemsReaderIntf::Ptr reader = StringXMLreader::create(report);
    reader->setPassPhrase(m_passPhrase);
    if (reader->first()){
        if (reader->readItem(this)){
            m_fileName = "";
            m_reportName = name;
            emit loadFinished();
            return true;
        };
    }
    return false;
}

bool ReportEnginePrivate::saveToFile(const QString &fileName)
{
    if (fileName.isEmpty() & m_fileName.isEmpty()) return false;
    QString fn = fileName.isEmpty() ? m_fileName : fileName;
    QFileInfo fi(fn);

    if (fi.suffix().isEmpty())
        fn+=".lrxml";

    QString dbSettingFileName = fi.absolutePath()+"/"+fi.baseName()+".db";
    QSettings dbcredentals(dbSettingFileName, QSettings::IniFormat);

    foreach (ConnectionDesc* connection, dataManager()->conections()) {
        if (!connection->keepDBCredentials()){
            dbcredentals.beginGroup(connection->name());
            dbcredentals.setValue("user",connection->userName());
            dbcredentals.setValue("password",connection->password());
            dbcredentals.endGroup();
            connection->setPassword("");
            connection->setUserName("");
        }
    }

    QScopedPointer< ItemsWriterIntf > writer(new XMLWriter());
    writer->setPassPhrase(m_passPhrase);
    writer->putItem(this);
    m_fileName=fn;   
    bool saved = writer->saveToFile(fn);

    foreach (ConnectionDesc* connection, dataManager()->conections()) {
        if (!connection->keepDBCredentials()){
            dbcredentals.beginGroup(connection->name());
            connection->setUserName(dbcredentals.value("user").toString());
            connection->setPassword(dbcredentals.value("password").toString());
            dbcredentals.endGroup();
        }
    }

    if (saved){
        foreach(PageDesignIntf* page, m_pages){
            page->setToSaved();
        }
    }
    dropChanges();
    this->setReportName(fi.baseName());
    return saved;
}

QByteArray ReportEnginePrivate::saveToByteArray()
{
    QScopedPointer< ItemsWriterIntf > writer(new XMLWriter());
    writer->setPassPhrase(m_passPhrase);
    writer->putItem(this);
    QByteArray result = writer->saveToByteArray();
    if (!result.isEmpty()){
        foreach(PageDesignIntf* page, m_pages){
            page->setToSaved();
        }
    }
    dropChanges();
    return result;
}

QString ReportEnginePrivate::saveToString(){
    QScopedPointer< ItemsWriterIntf > writer(new XMLWriter());
    writer->setPassPhrase(m_passPhrase);
    writer->putItem(this);
    QString result = writer->saveToString();
    if (!result.isEmpty()){
        foreach(PageDesignIntf* page, m_pages){
            page->setToSaved();
        }
    }
    dropChanges();
    return result;
}

bool ReportEnginePrivate::isNeedToSave()
{
    foreach(PageDesignIntf* page, m_pages){
        if (page->isHasChanges()) return true;
    }
    if (dataManager()->hasChanges()){
        return true;
    }
    if (scriptContext()->hasChanges())
        return true;
    return false;
}

QString ReportEnginePrivate::renderToString()
{
    LimeReport::ReportRender render;
    updateTranslations();
    dataManager()->connectAllDatabases();
    dataManager()->setDesignTime(false);
    if (m_pages.count()){
        render.setDatasources(dataManager());
        render.setScriptContext(scriptContext());
        return render.renderPageToString(m_pages.at(0)->pageItem());
    } else return QString();

}

ScaleType ReportEnginePrivate::previewScaleType()
{
    return m_previewScaleType;
}

int ReportEnginePrivate::previewScalePercent()
{
    return m_previewScalePercent;
}

void ReportEnginePrivate::setPreviewScaleType(const ScaleType &scaleType, int percent)
{
    m_previewScaleType = scaleType;
    m_previewScalePercent = percent;
}

void ReportEnginePrivate::addWatermark(const WatermarkSetting &watermarkSetting)
{
    m_watermarks.append(watermarkSetting);
}

void ReportEnginePrivate::clearWatermarks()
{
    m_watermarks.clear();
}

PageItemDesignIntf* ReportEnginePrivate::getPageByName(const QString& pageName)
{
    foreach(PageItemDesignIntf* page, m_renderingPages){
        if ( page->objectName().compare(pageName, Qt::CaseInsensitive) == 0)
            return page;
    }
    return 0;
}

IPreparedPages *ReportEnginePrivate::preparedPages(){
    return m_preparedPagesManager;
}

bool ReportEnginePrivate::showPreparedPages(PreviewHints hints, bool modal)
{
    return showPreviewWindow(m_preparedPages, hints, modal);
}

bool ReportEnginePrivate::prepareReportPages()
{
    try{
        dataManager()->setDesignTime(false);
        m_preparedPages = renderToPages();
        dataManager()->setDesignTime(true);
    } catch (ReportError &exception){
        saveError(exception.what());
        showError(exception.what());
        return false;
    }
    return !m_preparedPages.isEmpty();
}

bool ReportEnginePrivate::printPreparedPages()
{
    static QTemporaryDir tempDir;
    if (!tempDir.isValid()) return false;
    QString fileName = QDir(tempDir.path()).filePath(QStringLiteral("prepared.pdf"));
    if (!printPagesToPDF(m_preparedPages, fileName)) return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

Qt::LayoutDirection ReportEnginePrivate::previewLayoutDirection()
{
    return m_previewLayoutDirection;
}

void ReportEnginePrivate::setPreviewLayoutDirection(const Qt::LayoutDirection& layoutDirection)
{
    m_previewLayoutDirection = layoutDirection;
}

void ReportEnginePrivate::setPassPhrase(const QString &passPhrase)
{
    m_passPhrase = passPhrase;
}

void ReportEnginePrivate::reorderPages(const QList<PageDesignIntf *>& reorderedPages)
{
    m_pages.clear();
    foreach(PageDesignIntf* page, reorderedPages){
        m_pages.append(page);
    }
}

void ReportEnginePrivate::clearSelection()
{
    foreach (PageDesignIntf* page, m_pages) {
        foreach(GraphicsItem* item, page->selectedItems()){
            item->setSelected(false);
        }
    }
}

bool ReportEnginePrivate::addTranslationLanguage(QLocale::Language language)
{
    if (!m_translations.keys().contains(language)){
        ReportTranslation* translation = 0;
        if (!m_translations.contains(QLocale::AnyLanguage)){
            translation = new ReportTranslation(QLocale::AnyLanguage,m_pages);
            m_translations.insert(QLocale::AnyLanguage,translation);
        }
        translation = new ReportTranslation(language,m_pages);
        m_translations.insert(language, translation);
        return true;
    } else {
        m_lastError =  tr("Language %1 already exists").arg(QLocale::languageToString(language));
        return false;
    }
}

bool ReportEnginePrivate::removeTranslationLanguage(QLocale::Language language)
{
    return m_translations.remove(language) != 0;
}

void ReportEnginePrivate::activateLanguage(QLocale::Language language)
{
    if (!m_translations.keys().contains(language)) return;
    ReportTranslation* translation = m_translations.value(language);

    foreach(PageTranslation* pageTranslation, translation->pagesTranslation()){
        PageItemDesignIntf* page = getPageByName(pageTranslation->pageName);
        if (page){
            foreach(ItemTranslation* itemTranslation, pageTranslation->itemsTranslation){
                BaseDesignIntf* item = page->childByName(itemTranslation->itemName);
                if (item) {
                    foreach(PropertyTranslation* propertyTranslation, itemTranslation->propertyesTranslation){
                        if (propertyTranslation->checked)
                            item->setProperty(propertyTranslation->propertyName.toLatin1(), propertyTranslation->value);
                    }
                }
            }
        }
    }
}

QList<QLocale::Language> ReportEnginePrivate::designerLanguages()
{

    QList<QLocale::Language> result;
    emit getAvailableDesignerLanguages(&result);
    return result;
}

QLocale::Language ReportEnginePrivate::currentDesignerLanguage()
{
    QLocale::Language result = emit getCurrentDefaultDesignerLanguage();
    return result;
}

void ReportEnginePrivate::setCurrentDesignerLanguage(QLocale::Language language)
{
    m_currentDesignerLanguage = language;
    emit currentDefaultDesignerLanguageChanged(language);
}

QString ReportEnginePrivate::styleSheet() const
{
    return m_styleSheet;
}

void ReportEnginePrivate::setStyleSheet(const QString &styleSheet)
{
    m_styleSheet = styleSheet;
}

bool ReportEnginePrivate::setReportLanguage(QLocale::Language language){
    m_reportLanguage = language;
    if (!m_translations.keys().contains(language)) return false;
    //    activateLanguage(language);
    return true;
}

QList<QLocale::Language> ReportEnginePrivate::aviableLanguages()
{
    return  m_translations.keys();
}

ReportTranslation*ReportEnginePrivate::reportTranslation(QLocale::Language language)
{
    return m_translations.value(language);
}

bool ReportEnginePrivate::resultIsEditable() const
{
    return m_resultIsEditable;
}

void ReportEnginePrivate::setResultEditable(bool value)
{
    m_resultIsEditable = value;
}

bool ReportEnginePrivate::saveToFileIsVisible() const
{
    return m_saveToFileVisible;
}

void ReportEnginePrivate::setSaveToFileVisible(bool value)
{
    m_saveToFileVisible = value;
}

bool ReportEnginePrivate::printToPdfIsVisible() const
{
    return m_printToPdfVisible;
}

void ReportEnginePrivate::setPrintToPdfVisible(bool value)
{
    m_printToPdfVisible = value;
}

bool ReportEnginePrivate::printIsVisible() const
{
    return m_printVisible;
}

void ReportEnginePrivate::setPrintVisible(bool value)
{
    m_printVisible = value;
}

bool ReportEnginePrivate::suppressFieldAndVarError() const
{
    return m_reportSettings.suppressAbsentFieldsAndVarsWarnings();
}

void ReportEnginePrivate::setSuppressFieldAndVarError(bool suppressFieldAndVarError)
{
    m_reportSettings.setSuppressAbsentFieldsAndVarsWarnings(suppressFieldAndVarError);
}

bool ReportEnginePrivate::isBusy()
{
    return m_reportRendering;
}

QString ReportEnginePrivate::previewWindowTitle() const
{
    return m_previewWindowTitle;
}

void ReportEnginePrivate::setPreviewWindowTitle(const QString &previewWindowTitle)
{
    m_previewWindowTitle = previewWindowTitle;
}

QColor ReportEnginePrivate::previewWindowPageBackground()
{
    return m_previewPageBackgroundColor;
}

void ReportEnginePrivate::setPreviewWindowPageBackground(QColor color)
{
    m_previewPageBackgroundColor = color;
}

QIcon ReportEnginePrivate::previewWindowIcon() const
{
    return m_previewWindowIcon;
}

void ReportEnginePrivate::setPreviewWindowIcon(const QIcon &previewWindowIcon)
{
    m_previewWindowIcon = previewWindowIcon;
}

PageItemDesignIntf* ReportEnginePrivate::createRenderingPage(PageItemDesignIntf* page){
    PageItemDesignIntf* result = dynamic_cast<PageItemDesignIntf*>(page->cloneItem(page->itemMode()));
    ICollectionContainer* co = dynamic_cast<ICollectionContainer*>(result);
    if (co) co->collectionLoadFinished("children");
    return result;
}

void ReportEnginePrivate::initReport()
{
    for(int index = 0; index < pageCount(); ++index){
        PageDesignIntf* page =  pageAt(index);
        if (page != 0){
            foreach (BaseDesignIntf* item, page->pageItem()->childBaseItems()) {
                IPainterProxy *proxyItem = dynamic_cast<IPainterProxy *>(item);
                if (proxyItem){
                    proxyItem->setExternalPainter(this);
                }
            }
        }
    }
}

void ReportEnginePrivate::paintByExternalPainter(const QString& objectName, QPainter* painter, const StyleOptionGraphicsItem* options)
{
    emit externalPaint(objectName, painter, options);
}

BaseDesignIntf* ReportEnginePrivate::createWatermark(PageDesignIntf* page, WatermarkSetting watermarkSetting)
{

    WatermarkHelper watermarkHelper(watermarkSetting);

    BaseDesignIntf* watermark = page->addReportItem("TextItem", watermarkHelper.mapToPage(*page->pageItem()), watermarkHelper.sceneSize());
    if (watermark){
        watermark->setProperty("content", watermarkSetting.text());
        watermark->setProperty("font",watermarkSetting.font());
        watermark->setProperty("watermark",true);
        watermark->setProperty("itemLocation",1);
        watermark->setProperty("foregroundOpacity", watermarkSetting.opacity());
        watermark->setProperty("fontColor", watermarkSetting.color());
    }
    return watermark;

}

void ReportEnginePrivate::clearRenderingPages(){
    qDeleteAll(m_renderingPages.begin(), m_renderingPages.end());
    m_renderingPages.clear();
}

ReportPages ReportEnginePrivate::renderToPages()
{
    int startTOCPage = -1;
    int pageAfterTOCIndex = -1;

    if (m_reportRendering) return ReportPages();
    initReport();
    m_reportRender = ReportRender::Ptr(new ReportRender);
    updateTranslations();
    connect(m_reportRender.data(),SIGNAL(pageRendered(int)),
            this, SIGNAL(renderPageFinished(int)));

    if (m_pages.count()){

        ReportPages result;
        m_reportRendering = true;
        m_reportRender->setDatasources(dataManager());
        m_reportRender->setScriptContext(scriptContext());
        clearRenderingPages();
        foreach (PageDesignIntf* page, m_pages) {

            QVector<BaseDesignIntf*> watermarks;
            if (!m_watermarks.isEmpty()){
                foreach(WatermarkSetting watermarkSetting, m_watermarks){
                    watermarks.append(createWatermark(page, watermarkSetting));
                }
            }

            PageItemDesignIntf* rp = createRenderingPage(page->pageItem());


            qDeleteAll(watermarks.begin(),watermarks.end());
            watermarks.clear();

            m_renderingPages.append(rp);
            scriptContext()->baseDesignIntfToScript(rp->objectName(), rp);
        }

        scriptContext()->qobjectToScript("engine",this);
        if (m_scriptEngineContext->runInitScript()){

            dataManager()->clearErrors();
            dataManager()->connectAllDatabases();
            dataManager()->setDesignTime(false);
            dataManager()->updateDatasourceModel();

            activateLanguage(m_reportLanguage);
            emit renderStarted();
            m_scriptEngineContext->setReportPages(&result);

            for(int i = 0; i < m_renderingPages.count(); ++i){
                PageItemDesignIntf* page = m_renderingPages.at(i);
                if (!page->isTOC() && page->isPrintable()){
                    page->setReportSettings(&m_reportSettings);
                    result = appendPages(
                                result,
                                m_reportRender->renderPageToPages(page),
                                page->mixWithPriorPage() ? MixPages : AppendPages
                             );
                } else if (page->isTOC()){
                    startTOCPage = result.count();
                    pageAfterTOCIndex = i+1;
                    m_reportRender->createTOCMarker(page->resetPageNumber());
                }
            }

            for (int i=0; i<m_renderingPages.count(); ++i){
                PageItemDesignIntf* page = m_renderingPages.at(i);
                if (page->isTOC()){
                    page->setReportSettings(&m_reportSettings);
                    if (i < m_renderingPages.count()){
                        PageItemDesignIntf* secondPage = 0;
                        if ( m_renderingPages.count() > (pageAfterTOCIndex))
                            secondPage = m_renderingPages.at(pageAfterTOCIndex);
                        ReportPages pages = m_reportRender->renderTOC(
                                    page,
                                    true,
                                    secondPage && secondPage->resetPageNumber()
                        );
                        for (int j=0; j<pages.count(); ++j){
                            result.insert(startTOCPage+j,pages.at(j));
                        }

                    } else {
                        result = appendPages(
                                    result,
                                    m_reportRender->renderPageToPages(page),
                                    page->mixWithPriorPage() ? MixPages : AppendPages
                                 );
                    }
                }
            }

            m_reportRender->secondRenderPass(result);

            emit renderFinished();
            m_reportRender.clear();
            clearRenderingPages();
        }
        m_reportRendering = false;

        return result;
    } else {
        return ReportPages();
    }
}

ReportPages ReportEnginePrivate::appendPages(ReportPages s1, ReportPages s2, AppendType appendType)
{
    if (!s1.isEmpty() && s1.size() == s2.size() && appendType == MixPages){
        ReportPages result;
        ReportPages::Iterator s1It;
        ReportPages::Iterator s2It;
        for (s1It = s1.begin(), s2It = s2.begin(); s1It != s1.end(); ++s1It,++s2It){
            result.append(*s1It);
            result.append(*s2It);
        }
        return result;
    } else {
        s1.append(s2);
        return s1;
    }
}

QString ReportEnginePrivate::lastError()
{
    return m_lastError;
}

ReportEngine::ReportEngine(QObject *parent)
    : QObject(parent), d_ptr(new ReportEnginePrivate()), m_showDesignerModal(true), m_showPreviewModal(true)
{
    Q_D(ReportEngine);
    d->q_ptr=this;
    connect(d, SIGNAL(renderStarted()), this, SIGNAL(renderStarted()));
    connect(d, SIGNAL(renderPageFinished(int)),
            this, SIGNAL(renderPageFinished(int)));
    connect(d, SIGNAL(renderFinished()), this, SIGNAL(renderFinished()));

    connect(d, SIGNAL(printingStarted(int)), this, SIGNAL(printingStarted(int)));
    connect(d, SIGNAL(pagePrintingFinished(int)),
            this, SIGNAL(pagePrintingFinished(int)));
    connect(d, SIGNAL(printingFinished()), this, SIGNAL(printingFinished()));

    connect(d, SIGNAL(onSave(bool&)), this, SIGNAL(onSave(bool&)));
    connect(d, SIGNAL(onSaveAs(bool&)), this, SIGNAL(onSaveAs(bool&)));
    connect(d, SIGNAL(onLoad(bool&)), this, SIGNAL(onLoad(bool&)));
    connect(d, SIGNAL(saveFinished()), this, SIGNAL(saveFinished()));
    connect(d, SIGNAL(loadFinished()), this, SIGNAL(loadFinished()));
    connect(d, SIGNAL(cleared()), this, SIGNAL(cleared()));
    connect(d, SIGNAL(printedToPDF(QString)), this, SIGNAL(printedToPDF(QString)));
    
    connect(d, SIGNAL(getAvailableDesignerLanguages(QList<QLocale::Language>*)),
            this, SIGNAL(getAvailableDesignerLanguages(QList<QLocale::Language>*)));
    connect(d, SIGNAL(currentDefaultDesignerLanguageChanged(QLocale::Language)),
            this, SIGNAL(currentDefaultDesignerLanguageChanged(QLocale::Language)));
    connect(d, SIGNAL(getCurrentDefaultDesignerLanguage()),
            this, SIGNAL(getCurrentDefaultDesignerLanguage()));

    connect(d, SIGNAL(externalPaint(const QString&, QPainter*, const StyleOptionGraphicsItem*)),
            this, SIGNAL(externalPaint(const QString&, QPainter*, const StyleOptionGraphicsItem*)));
    connect(d, SIGNAL(onSavePreview(bool&, LimeReport::IPreparedPages*)),
            this, SIGNAL(onSavePreview(bool&, LimeReport::IPreparedPages*)));
}

ReportEngine::~ReportEngine()
{
    delete d_ptr;
}

bool ReportEngine::printReport()
{
    Q_D(ReportEngine);
    return d->printReport();
}

bool ReportEngine::printPagesToPDF(ReportPages pages, const QString& fileName, const PrintRange& range)
{
    Q_D(ReportEngine);
    return d->printPagesToPDF(pages, fileName, range);
}

QList<QImage> ReportEngine::renderToImages(qreal dpi)
{
    Q_D(ReportEngine);
    return d->renderToImages(dpi);
}

void ReportEngine::printToFile(const QString &fileName)
{
    Q_D(ReportEngine);
    d->printToFile(fileName);
}

bool ReportEngine::printToPDF(const QString &fileName, const PrintRange& range)
{
    Q_D(ReportEngine);
    return d->printToPDF(fileName, range);
}

bool ReportEngine::exportReport(QString exporterName, const QString &fileName, const QMap<QString, QVariant> &params)
{
    Q_D(ReportEngine);
    return d->exportReport(exporterName, fileName, params);
}

void ReportEngine::previewReport(PreviewHints hints)
{
    Q_D(ReportEngine);
    if (m_settings)
        d->setSettings(m_settings);
    d->previewReport(hints, m_showPreviewModal);
}

void ReportEngine::designReport()
{
    Q_D(ReportEngine);
    if (m_settings)
        d->setSettings(m_settings);
    d->designReport(showDesignerModal());
}

void ReportEngine::setPreviewWindowTitle(const QString &title)
{
    Q_D(ReportEngine);
    d->setPreviewWindowTitle(title);
}

void ReportEngine::setPreviewWindowIcon(const QIcon &icon)
{
    Q_D(ReportEngine);
    d->setPreviewWindowIcon(icon);
}

void ReportEngine::setPreviewPageBackgroundColor(QColor color)
{
    Q_D(ReportEngine);
    d->setPreviewWindowPageBackground(color);
}

void ReportEngine::setResultEditable(bool value)
{
    Q_D(ReportEngine);
    d->setResultEditable(value);
}

bool ReportEngine::resultIsEditable()
{
    Q_D(ReportEngine);
    return d->resultIsEditable();
}

void ReportEngine::setSaveToFileVisible(bool value)
{
    Q_D(ReportEngine);
    d->setSaveToFileVisible(value);
}

bool ReportEngine::saveToFileIsVisible()
{
    Q_D(ReportEngine);
    return d->saveToFileIsVisible();
}

void ReportEngine::setPrintToPdfVisible(bool value)
{
    Q_D(ReportEngine);
    d->setPrintToPdfVisible(value);
}

bool ReportEngine::printToPdfIsVisible()
{
    Q_D(ReportEngine);
    return d->printToPdfIsVisible();
}

void ReportEngine::setPrintVisible(bool value)
{
    Q_D(ReportEngine);
    d->setPrintVisible(value);
}

bool ReportEngine::printIsVisible()
{
    Q_D(ReportEngine);
    return d->printIsVisible();
}

bool ReportEngine::isBusy()
{
    Q_D(ReportEngine);
    return d->isBusy();
}

void ReportEngine::setPassPhrase(QString &passPhrase)
{
    Q_D(ReportEngine);
    d->setPassPhrase(passPhrase);
}

QList<QLocale::Language> ReportEngine::availableLanguages()
{
    Q_D(ReportEngine);
    return d->aviableLanguages();
}

bool ReportEngine::setReportLanguage(QLocale::Language language)
{
    Q_D(ReportEngine);
    return d->setReportLanguage(language);
}

Qt::LayoutDirection ReportEngine::previewLayoutDirection()
{
    Q_D(ReportEngine);
    return d->previewLayoutDirection();
}

void ReportEngine::setPreviewLayoutDirection(const Qt::LayoutDirection& previewLayoutDirection)
{
    Q_D(ReportEngine);
    d->setPreviewLayoutDirection(previewLayoutDirection);
}

QList<QLocale::Language> ReportEngine::designerLanguages()
{
    Q_D(ReportEngine);
    return d->designerLanguages();
}

QLocale::Language ReportEngine::currentDesignerLanguage()
{
    Q_D(ReportEngine);
    return d->currentDesignerLanguage();
}

ScaleType ReportEngine::previewScaleType()
{
    Q_D(ReportEngine);
    return d->previewScaleType();
}

int ReportEngine::previewScalePercent()
{
    Q_D(ReportEngine);
    return d->previewScalePercent();
}

void ReportEngine::setPreviewScaleType(const ScaleType &previewScaleType, int percent)
{
    Q_D(ReportEngine);
    d->setPreviewScaleType(previewScaleType, percent);
}

void ReportEngine::addWatermark(const WatermarkSetting &watermarkSetting)
{
    Q_D(ReportEngine);
    d->addWatermark(watermarkSetting);
}

void ReportEngine::clearWatermarks()
{
    Q_D(ReportEngine);
    d->clearWatermarks();
}

IPreparedPages *ReportEngine::preparedPages()
{
    Q_D(ReportEngine);
    return d->preparedPages();
}

bool ReportEngine::showPreparedPages(PreviewHints hints)
{
    Q_D(ReportEngine);
    return d->showPreparedPages(hints, m_showPreviewModal);
}

bool ReportEngine::prepareReportPages()
{
    Q_D(ReportEngine);
    return d->prepareReportPages();
}

bool ReportEngine::printPreparedPages()
{
    Q_D(ReportEngine);
    return d->printPreparedPages();
}

void ReportEngine::setShowProgressDialog(bool value)
{
    Q_D(ReportEngine);
    d->setShowProgressDialog(value);
}

bool ReportEngine::isShowProgressDialog()
{
    Q_D(ReportEngine);
    return d->isShowProgressDialog();
}

IDataSourceManager *ReportEngine::dataManager()
{
    Q_D(ReportEngine);
    return d->dataManagerIntf();
}

IScriptEngineManager *ReportEngine::scriptManager()
{
    Q_D(ReportEngine);
    return d->scriptManagerIntf();
}

bool ReportEngine::loadFromFile(const QString &fileName, bool autoLoadPreviewOnChange)
{
    Q_D(ReportEngine);
    return d->loadFromFile(fileName, autoLoadPreviewOnChange);
}

static QString urlToFileName(const QUrl& url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

bool ReportEngine::loadFromUrl(const QUrl& fileUrl, bool autoLoadPreviewOnChange)
{
    return loadFromFile(urlToFileName(fileUrl), autoLoadPreviewOnChange);
}

bool ReportEngine::saveToUrl(const QUrl& fileUrl)
{
    return saveToFile(urlToFileName(fileUrl));
}

bool ReportEngine::printToPdfUrl(const QUrl& fileUrl)
{
    return printToPDF(urlToFileName(fileUrl));
}

bool ReportEngine::loadFromByteArray(QByteArray* data){
    Q_D(ReportEngine);
    return d->loadFromByteArray(data);
}

bool ReportEngine::loadFromString(const QString &data)
{
    Q_D(ReportEngine);
    return d->loadFromString(data);
}

QString ReportEngine::reportFileName()
{
    Q_D(ReportEngine);
    return d->reportFileName();
}

void ReportEngine::setReportFileName(const QString &fileName)
{
    Q_D(ReportEngine);
    return d->setReportFileName(fileName);
}

bool ReportEngine::saveToFile(const QString &fileName)
{
    Q_D(ReportEngine);
    return d->saveToFile(fileName);
}

QByteArray ReportEngine::saveToByteArray()
{
    Q_D(ReportEngine);
    return d->saveToByteArray();
}

QString ReportEngine::saveToString()
{
    Q_D(ReportEngine);
    return d->saveToString();
}

QString ReportEngine::lastError()
{
    Q_D(ReportEngine);
    return d->lastError();
}

void ReportEngine::setCurrentReportsDir(const QString &dirName)
{
    Q_D(ReportEngine);
    return d->setCurrentReportsDir(dirName);
}

void ReportEngine::setReportName(const QString &name)
{
    Q_D(ReportEngine);
    return d->setReportName(name);
}

QString ReportEngine::reportName()
{
    Q_D(ReportEngine);
    return d->reportName();
}

void ReportEngine::cancelRender()
{
    Q_D(ReportEngine);
    d->cancelRender();
}

void ReportEngine::cancelPrinting()
{
    Q_D(ReportEngine);
    d->cancelPrinting();
}

ReportEngine::ReportEngine(ReportEnginePrivate &dd, QObject *parent)
    :QObject(parent), d_ptr(&dd), m_showDesignerModal(true), m_showPreviewModal(true)
{
    Q_D(ReportEngine);
    d->q_ptr=this;
    connect(d, SIGNAL(renderStarted()), this, SIGNAL(renderStarted()));
    connect(d, SIGNAL(renderPageFinished(int)),
            this, SIGNAL(renderPageFinished(int)));
    connect(d, SIGNAL(renderFinished()), this, SIGNAL(renderFinished()));
}

bool ReportEngine::showDesignerModal() const
{
    return m_showDesignerModal;
}

void ReportEngine::setShowDesignerModal(bool showDesignerModal)
{
    m_showDesignerModal = showDesignerModal;
}

bool ReportEngine::showPreviewModal() const
{
    return m_showPreviewModal;
}

void ReportEngine::setShowPreviewModal(bool value)
{
    m_showPreviewModal = value;
}

ScriptEngineManager*LimeReport::ReportEnginePrivate::scriptManager(){
    ScriptEngineManager::instance().setContext(scriptContext());
    ScriptEngineManager::instance().setDataManager(dataManager());
    return &ScriptEngineManager::instance();
}

PdfPrintProcessor::PdfPrintProcessor(QPdfWriter* writer)
    : m_writer(writer), m_painter(0), m_firstPage(true)
{}

PdfPrintProcessor::~PdfPrintProcessor()
{
    finish();
}

void PdfPrintProcessor::finish()
{
    if (m_painter){
        m_painter->end();
        delete m_painter;
        m_painter = 0;
    }
}

QPageLayout PdfPrintProcessor::pageLayout(PageItemDesignIntf* page) const
{
    QPageLayout::Orientation orientation = static_cast<QPageLayout::Orientation>(page->pageOrientation());
    QPageSize pageSize;
    if (page->pageSize() == PageItemDesignIntf::Custom || page->oldPrintMode()){
        QSizeF size = (page->pageOrientation() == PageItemDesignIntf::Landscape) ?
                    QSizeF(page->sizeMM().height(), page->sizeMM().width()) :
                    page->sizeMM();
        pageSize = QPageSize(size, QPageSize::Millimeter, QString(), QPageSize::ExactMatch);
    } else {
        pageSize = QPageSize(static_cast<QPageSize::PageSizeId>(page->pageSize()));
    }
    // Report pages already contain their margins, so the PDF page is used in full.
    return QPageLayout(pageSize, orientation, QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter);
}

void PdfPrintProcessor::renderPage(PageItemDesignIntf::Ptr page, QPainter* painter, const QRectF& target)
{
    PageDesignIntf renderPage;
    renderPage.setItemMode(PrintMode);
    PageDesignIntf* backupPage = dynamic_cast<PageDesignIntf*>(page->scene());
    QPointF backupPagePos = page->pos();
    page->setPos(0,0);
    renderPage.setPageItem(page);
    QRectF source = renderPage.pageItem()->mapToScene(renderPage.pageItem()->rect()).boundingRect();
    renderPage.setSceneRect(source);
    renderPage.setBackgroundBrush(Qt::NoBrush);
    renderPage.render(painter, target, source, Qt::IgnoreAspectRatio);
    page->setPos(backupPagePos);
    renderPage.removePageItem(page);
    if (backupPage) backupPage->reactivatePageItem(page);
}

bool PdfPrintProcessor::printPage(PageItemDesignIntf::Ptr page)
{
    QPageLayout layout = pageLayout(page.data());
    if (m_firstPage){
        m_writer->setPageLayout(layout);
        m_painter = new QPainter(m_writer);
        if (!m_painter->isActive()) return false;
        m_firstPage = false;
    } else {
        m_writer->setPageLayout(layout);
        if (!m_writer->newPage()) return false;
    }
    QRectF target(QPointF(0, 0), layout.fullRectPixels(m_writer->resolution()).size());
    renderPage(page, m_painter, target);
    return true;
}

qreal ItemGeometry::x() const
{
    return m_x;
}

void ItemGeometry::setX(const qreal &x)
{
    m_x = x;
}

qreal ItemGeometry::y() const
{
    return m_y;
}

void ItemGeometry::setY(const qreal &y)
{
    m_y = y;
}

qreal ItemGeometry::width() const
{
    return m_width;
}

void ItemGeometry::setWidth(const qreal &width)
{
    m_width = width;
}

qreal ItemGeometry::height() const
{
    return m_height;
}

void ItemGeometry::setHeight(const qreal &height)
{
    m_height = height;
}

ItemGeometry::Type ItemGeometry::type() const
{
    return m_type;
}

void ItemGeometry::setType(const Type &type)
{
    m_type = type;
}

Qt::Alignment ItemGeometry::anchor() const
{
    return m_anchor;
}

void ItemGeometry::setAnchor(const Qt::Alignment &anchor)
{
    m_anchor = anchor;
}

QString WatermarkSetting::text() const
{
    return m_text;
}

void WatermarkSetting::setText(const QString &text)
{
    m_text = text;
}

QFont WatermarkSetting::font() const
{
    return m_font;
}

void WatermarkSetting::setFont(const QFont &font)
{
    m_font = font;
}

int WatermarkSetting::opacity() const
{
    return m_opacity;
}

void WatermarkSetting::setOpacity(const int &opacity)
{
    m_opacity = opacity;
}

ItemGeometry WatermarkSetting::geometry() const
{
    return m_geometry;
}

void WatermarkSetting::setGeometry(const ItemGeometry &geometry)
{
    m_geometry = geometry;
}

QColor WatermarkSetting::color() const
{
    return m_color;
}

void WatermarkSetting::setColor(const QColor &color)
{
    m_color = color;
}

qreal WatermarkHelper::sceneX()
{
    return valueToPixels(m_watermark.geometry().x());
}

qreal WatermarkHelper::sceneY()
{
    return valueToPixels(m_watermark.geometry().y());
}

qreal WatermarkHelper::sceneWidth()
{
    return valueToPixels(m_watermark.geometry().width());
}

qreal WatermarkHelper::sceneHeight()
{
    return valueToPixels(m_watermark.geometry().height());
}

QPointF WatermarkHelper::scenePos()
{
    return (QPointF(sceneX(), sceneY()));
}

QSizeF WatermarkHelper::sceneSize()
{
    return (QSizeF(sceneWidth(), sceneHeight()));
}

QPointF WatermarkHelper::mapToPage(const PageItemDesignIntf &page)
{
    qreal startX = 0;
    qreal startY = 0;

    if ( m_watermark.geometry().anchor() & Qt::AlignLeft){
        startX = 0;
    } else if ( m_watermark.geometry().anchor() & Qt::AlignRight){
        startX = page.geometry().width();
    } else {
        startX = page.geometry().width() / 2;
    }

    if ( m_watermark.geometry().anchor() & Qt::AlignTop){
        startY = 0;
    } else if (m_watermark.geometry().anchor() & Qt::AlignBottom){
        startY = page.geometry().height();
    } else {
        startY = page.geometry().height() / 2;
    }

    return QPointF(startX + sceneX(), startY + sceneY());
}

qreal WatermarkHelper::valueToPixels(qreal value)
{
    switch (m_watermark.geometry().type()) {
    case LimeReport::ItemGeometry::Millimeters:
        return value * Const::mmFACTOR;
    case LimeReport::ItemGeometry::Pixels:
        return value;
    }
    return -1;
}


}// namespace LimeReport

