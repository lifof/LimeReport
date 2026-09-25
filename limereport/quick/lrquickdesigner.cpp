#include "lrquickdesigner.h"

#include "lrbanddesignintf.h"
#include "lrdatadesignintf.h"
#include "lrdatasourcemanager.h"
#include "lrdesignelementsfactory.h"
#include "lrmessagehub.h"
#include "lrpagedesignintf.h"
#include "lrquickdatamodel.h"
#include "lrquickpropertymodel.h"
#include "lrquickwindows.h"
#include "lrreportengine_p.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaEnum>
#include <QSqlDatabase>

namespace LimeReport {

QuickReportDesigner::QuickReportDesigner(QObject* parent):
    QObject(parent),
    m_currentPageIndex(-1),
    m_propertyModel(new QuickPropertyModel(this)),
    m_dataModel(new QuickDataBrowserModel(this)),
    m_objectTreeModel(new QuickObjectTreeModel(this)),
    m_magneticMovement(false),
    m_useGrid(false),
    m_gridStep(10),
    m_layoutEditMode(false)
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(80);
    connect(&m_refreshTimer, &QTimer::timeout, m_propertyModel, &QuickPropertyModel::refresh);
    connect(MessageHub::instance(), &MessageHub::messagePosted, this,
            &QuickReportDesigner::message);
}

QuickReportDesigner::~QuickReportDesigner() { }

QObject* QuickReportDesigner::engineObject() const { return m_engine; }

ReportEnginePrivate* QuickReportDesigner::d() const { return m_engine ? m_engine->d_ptr : nullptr; }

DataSourceManager* QuickReportDesigner::dataManager() const
{
    return d() ? d()->dataManager() : nullptr;
}

void QuickReportDesigner::setEngineObject(QObject* engineObject)
{
    ReportEngine* engine = qobject_cast<ReportEngine*>(engineObject);
    if (m_engine == engine)
        return;
    if (m_engine) {
        disconnect(m_engine, nullptr, this, nullptr);
        disconnect(m_engine->d_ptr, nullptr, this, nullptr);
    }
    m_engine = engine;
    if (m_engine) {
        connect(m_engine, &QObject::destroyed, this, &QuickReportDesigner::onEngineDestroyed);
        connect(d(), &ReportEnginePrivate::loadFinished, this,
                &QuickReportDesigner::onReportLoaded);
        connect(d(), &ReportEnginePrivate::cleared, this, &QuickReportDesigner::onReportCleared);
        connect(d(), &ReportEnginePrivate::saveFinished, this, &QuickReportDesigner::reportChanged);
        connect(d(), &ReportEnginePrivate::saveFinished, this,
                &QuickReportDesigner::modifiedChanged);
        d()->dataManager()->setDesignTime(true);
        connect(d()->dataManager(), &DataSourceManager::datasourcesChanged, this,
                &QuickReportDesigner::dataChanged);
        connect(d()->dataManager(), &DataSourceManager::datasourcesChanged, this,
                &QuickReportDesigner::modifiedChanged);
        m_propertyModel->setDataManager(d()->dataManager());
        m_dataModel->setDataManager(d()->dataManager());
        if (d()->pageCount() == 0)
            createStartPage();
    } else {
        m_propertyModel->setDataManager(nullptr);
        m_dataModel->setDataManager(nullptr);
    }
    onReportLoaded();
    emit engineChanged();
}

void QuickReportDesigner::onEngineDestroyed()
{
    m_engine = nullptr;
    m_currentPageIndex = -1;
    m_propertyModel->setObjects(QList<QObject*>());
    m_objectTreeModel->setPage(nullptr);
    m_dataModel->setDataManager(nullptr);
    emit engineChanged();
    emit pagesChanged();
    emit currentPageChanged();
}

void QuickReportDesigner::createStartPage()
{
    if (!d())
        return;
    d()->appendPage("page1");
}

void QuickReportDesigner::connectPage(PageDesignIntf* page)
{
    if (!page)
        return;
    disconnect(page, nullptr, this, nullptr);
    connect(page, &GraphicsScene::selectionChanged, this,
            &QuickReportDesigner::onPageSelectionChanged);
    connect(page, &PageDesignIntf::commandHistoryChanged, this,
            &QuickReportDesigner::onHistoryChanged);
    connect(page, &PageDesignIntf::itemInserted, this, &QuickReportDesigner::onItemInserted);
    connect(page, &PageDesignIntf::itemInsertCanceled, this,
            &QuickReportDesigner::onInsertCanceled);
    connect(page, &PageDesignIntf::itemEditorRequested, this,
            &QuickReportDesigner::onItemEditorRequested);
    connect(page, &PageDesignIntf::itemPropertyChanged, this,
            &QuickReportDesigner::onPropertyChanged);
    connect(page, &GraphicsScene::changed, this, &QuickReportDesigner::onPropertyChanged);
    connect(page, &PageDesignIntf::itemPropertyObjectNameChanged, this,
            &QuickReportDesigner::pagesChanged);
    page->setMagneticMovement(m_magneticMovement);
    page->clearSelection();
}

void QuickReportDesigner::onReportCleared()
{
    // Pages are gone (a report is being loaded or a new one started);
    // onReportLoaded() rebuilds everything once the new content is there.
    m_currentPageIndex = -1;
    m_propertyModel->setObjects(QList<QObject*>());
    m_objectTreeModel->setPage(nullptr);
    emit currentPageChanged();
    emit pagesChanged();
}

void QuickReportDesigner::onReportLoaded()
{
    if (d()) {
        if (d()->pageCount() == 0)
            createStartPage();
        for (int i = 0; i < d()->pageCount(); ++i)
            connectPage(d()->pageAt(i));
        applyGrid();
        setLayoutEditMode(m_layoutEditMode);
        d()->dataManager()->setDesignTime(true);
        d()->dataManager()->updateDatasourceModel();
    }
    m_currentPageIndex = -1;
    emit pagesChanged();
    setCurrentPageIndex(0);
    m_dataModel->rebuild();
    emit reportChanged();
    emit scriptChanged();
    emit historyChanged();
    emit modifiedChanged();
    emit dataChanged();
}

QStringList QuickReportDesigner::pageNames() const
{
    QStringList result;
    if (!d())
        return result;
    for (int i = 0; i < d()->pageCount(); ++i)
        result << d()->pageAt(i)->pageItem()->objectName();
    return result;
}

void QuickReportDesigner::setCurrentPageIndex(int index)
{
    if (!d() || d()->pageCount() == 0)
        index = -1;
    else
        index = qBound(0, index, d()->pageCount() - 1);
    if (index == m_currentPageIndex)
        return;
    if (currentPage())
        currentPage()->startEditMode();
    m_currentPageIndex = index;
    m_objectTreeModel->setPage(currentPage());
    updateSelectionModels();
    emit currentPageChanged();
    emit historyChanged();
    emit selectionChanged();
}

PageDesignIntf* QuickReportDesigner::currentPage() const
{
    if (!d() || m_currentPageIndex < 0 || m_currentPageIndex >= d()->pageCount())
        return nullptr;
    return d()->pageAt(m_currentPageIndex);
}

QObject* QuickReportDesigner::scene() const { return currentPage(); }

QObject* QuickReportDesigner::propertyModel() const { return m_propertyModel; }
QObject* QuickReportDesigner::dataModel() const { return m_dataModel; }
QObject* QuickReportDesigner::objectTreeModel() const { return m_objectTreeModel; }

QString QuickReportDesigner::reportFileName() const
{
    return d() ? d()->reportFileName() : QString();
}

QString QuickReportDesigner::reportName() const
{
    if (!d())
        return QString();
    if (!d()->reportName().isEmpty())
        return d()->reportName();
    if (!d()->reportFileName().isEmpty())
        return QFileInfo(d()->reportFileName()).completeBaseName();
    return tr("New report");
}

bool QuickReportDesigner::modified() const { return d() ? d()->isNeedToSave() : false; }

bool QuickReportDesigner::canUndo() const
{
    return currentPage() ? currentPage()->isCanUndo() : false;
}

bool QuickReportDesigner::canRedo() const
{
    return currentPage() ? currentPage()->isCanRedo() : false;
}

int QuickReportDesigner::selectionCount() const
{
    return currentPage() ? currentPage()->selectedItems().count() : 0;
}

QObject* QuickReportDesigner::selectedObject() const
{
    if (!currentPage())
        return nullptr;
    QList<GraphicsItem*> items = currentPage()->selectedItems();
    if (items.count() != 1)
        return nullptr;
    return dynamic_cast<BaseDesignIntf*>(items.first());
}

QVariantList QuickReportDesigner::itemTypes() const
{
    QVariantList result;
    const auto attribs = DesignElementsFactory::instance().attribsMap();
    for (auto it = attribs.constBegin(); it != attribs.constEnd(); ++it) {
        if (it.value().m_tag.compare("Item", Qt::CaseInsensitive) != 0)
            continue;
        QVariantMap item;
        item.insert("type", it.key());
        item.insert("name", QObject::tr(it.value().m_alias.toLatin1()));
        item.insert("icon",
                    QFile::exists(":/items/" + it.key()) ? QString("qrc:/items/%1").arg(it.key())
                                                         : QString());
        result.append(item);
    }
    return result;
}

QVariantList QuickReportDesigner::bandTypes() const
{
    QVariantList result;
    PageDesignIntf* page = currentPage();
    BandDesignIntf* band = nullptr;
    if (page && page->selectedItems().count() == 1)
        band = dynamic_cast<BandDesignIntf*>(page->selectedItems().first());

    auto add = [&result](int type, const QString& name, bool enabled) {
        QVariantMap item;
        item.insert("type", type);
        item.insert("name", name);
        item.insert("enabled", enabled);
        result.append(item);
    };
    bool hasPage = page != nullptr;
    add(BandDesignIntf::ReportHeader, tr("Report Header"),
        hasPage && !page->pageItem()->isBandExists(BandDesignIntf::ReportHeader));
    add(BandDesignIntf::ReportFooter, tr("Report Footer"),
        hasPage && !page->pageItem()->isBandExists(BandDesignIntf::ReportFooter));
    add(BandDesignIntf::PageHeader, tr("Page Header"),
        hasPage && !page->pageItem()->isBandExists(BandDesignIntf::PageHeader));
    add(BandDesignIntf::PageFooter, tr("Page Footer"),
        hasPage && !page->pageItem()->isBandExists(BandDesignIntf::PageFooter));
    add(BandDesignIntf::Data, tr("Data"), hasPage);
    bool isData = band && band->bandType() == BandDesignIntf::Data;
    bool isSubDetail = band && band->bandType() == BandDesignIntf::SubDetailBand;
    bool isGroupHeader = band && band->bandType() == BandDesignIntf::GroupHeader;
    add(BandDesignIntf::DataHeader, tr("Data Header"),
        isData && !band->isConnectedToBand(BandDesignIntf::DataHeader));
    add(BandDesignIntf::DataFooter, tr("Data Footer"),
        isData && !band->isConnectedToBand(BandDesignIntf::DataFooter));
    add(BandDesignIntf::SubDetailBand, tr("SubDetail"), isData || isSubDetail);
    add(BandDesignIntf::SubDetailHeader, tr("SubDetail Header"),
        isSubDetail && !band->isConnectedToBand(BandDesignIntf::SubDetailHeader));
    add(BandDesignIntf::SubDetailFooter, tr("SubDetail Footer"),
        isSubDetail && !band->isConnectedToBand(BandDesignIntf::SubDetailFooter));
    add(BandDesignIntf::GroupHeader, tr("Group Header"),
        (isData && !band->isConnectedToBand(BandDesignIntf::GroupHeader)) || isSubDetail
            || (isGroupHeader && !band->isConnectedToBand(BandDesignIntf::GroupHeader)));
    add(BandDesignIntf::GroupFooter, tr("Group Footer"),
        isGroupHeader && !band->isConnectedToBand(BandDesignIntf::GroupFooter));
    add(BandDesignIntf::TearOffBand, tr("Tear-off Band"),
        hasPage && !page->pageItem()->isBandExists(BandDesignIntf::TearOffBand));
    return result;
}

QString QuickReportDesigner::script() const
{
    return d() ? d()->scriptContext()->initScript() : QString();
}

void QuickReportDesigner::setScript(const QString& script)
{
    if (!d() || d()->scriptContext()->initScript() == script)
        return;
    d()->scriptContext()->setInitScript(script);
    emit scriptChanged();
    emit modifiedChanged();
}

void QuickReportDesigner::setMagneticMovement(bool value)
{
    if (m_magneticMovement == value)
        return;
    m_magneticMovement = value;
    if (d())
        for (int i = 0; i < d()->pageCount(); ++i)
            d()->pageAt(i)->setMagneticMovement(value);
    emit settingsChanged();
}

void QuickReportDesigner::setUseGrid(bool value)
{
    if (m_useGrid == value)
        return;
    m_useGrid = value;
    applyGrid();
    emit settingsChanged();
}

void QuickReportDesigner::setGridStep(int value)
{
    value = qMax(1, value);
    if (m_gridStep == value)
        return;
    m_gridStep = value;
    applyGrid();
    emit settingsChanged();
}

void QuickReportDesigner::applyGrid()
{
    if (!d())
        return;
    int step = m_useGrid ? m_gridStep : Const::DEFAULT_GRID_STEP;
    for (int i = 0; i < d()->pageCount(); ++i) {
        d()->pageAt(i)->setVerticalGridStep(step);
        d()->pageAt(i)->setHorizontalGridStep(step);
    }
}

void QuickReportDesigner::setLayoutEditMode(bool value)
{
    bool changed = m_layoutEditMode != value;
    m_layoutEditMode = value;
    if (d()) {
        for (int i = 0; i < d()->pageCount(); ++i) {
            PageItemDesignIntf* pageItem = d()->pageAt(i)->pageItem();
            if (value)
                pageItem->setItemMode(pageItem->itemMode() | LayoutEditMode);
            else if (pageItem->itemMode() & LayoutEditMode)
                pageItem->setItemMode(pageItem->itemMode() ^ LayoutEditMode);
        }
    }
    if (changed)
        emit settingsChanged();
}

QStringList QuickReportDesigner::sqlDrivers() const { return QSqlDatabase::drivers(); }

QStringList QuickReportDesigner::connectionNames() const
{
    QStringList result;
    if (!dataManager())
        return result;
    QStringList names = QSqlDatabase::connectionNames();
    for (const QString& name : dataManager()->connectionNames())
        if (!names.contains(name))
            names.append(name);
    for (const QString& name : names)
        result << ConnectionDesc::connectionNameForUser(name);
    result.sort(Qt::CaseInsensitive);
    return result;
}

QStringList QuickReportDesigner::datasourceNames() const
{
    QStringList result = dataManager() ? dataManager()->dataSourceNames() : QStringList();
    result.sort(Qt::CaseInsensitive);
    return result;
}

QStringList QuickReportDesigner::fieldNames(const QString& datasourceName) const
{
    QStringList result = dataManager() ? dataManager()->fieldNames(datasourceName) : QStringList();
    result.sort(Qt::CaseInsensitive);
    return result;
}

QStringList QuickReportDesigner::variableTypes() const
{
    QStringList result;
    QMetaEnum e = QMetaEnum::fromType<Enums::VariableDataType>();
    for (int i = 0; i < e.keyCount(); ++i)
        result << QString::fromLatin1(e.key(i));
    return result;
}

void QuickReportDesigner::setLastError(const QString& error)
{
    m_lastError = error;
    emit lastErrorChanged();
}

// ----- report / files ------------------------------------------------------

void QuickReportDesigner::newReport()
{
    if (!d())
        return;
    d()->clearReport();
    d()->setReportFileName("");
    d()->setReportName("");
    d()->scriptContext()->setInitScript("");
    createStartPage();
    d()->dataManager()->dropChanges();
    d()->scriptContext()->dropChanges();
    onReportLoaded();
}

bool QuickReportDesigner::openReport(const QUrl& fileUrl)
{
    if (!d())
        return false;
    QString fileName = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    bool loaded = false;
    emit d()->onLoad(loaded);
    if (loaded)
        return true;
    if (!d()->loadFromFile(fileName, false)) {
        setLastError(tr("Wrong file format"));
        MessageHub::critical(nullptr, tr("Error"), tr("Wrong file format"));
        return false;
    }
    return true;
}

bool QuickReportDesigner::hasFileName() const { return d() && !d()->reportFileName().isEmpty(); }

bool QuickReportDesigner::saveReport()
{
    if (!d())
        return false;
    d()->clearSelection();
    if (d()->emitSaveReport()) {
        emit modifiedChanged();
        return true;
    }
    if (d()->reportFileName().isEmpty())
        return false;
    bool result = d()->saveToFile();
    if (result)
        d()->emitSaveFinished();
    else
        setLastError(tr("Can't save file %1").arg(d()->reportFileName()));
    emit reportChanged();
    emit modifiedChanged();
    emit historyChanged();
    return result;
}

bool QuickReportDesigner::saveReportAs(const QUrl& fileUrl)
{
    if (!d())
        return false;
    d()->clearSelection();
    if (d()->emitSaveReportAs()) {
        emit modifiedChanged();
        return true;
    }
    QString fileName = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (fileName.isEmpty())
        return false;
    bool result = d()->saveToFile(fileName);
    if (result)
        d()->emitSaveFinished();
    else
        setLastError(tr("Can't save file %1").arg(fileName));
    emit reportChanged();
    emit modifiedChanged();
    return result;
}

QUrl QuickReportDesigner::reportFolder() const
{
    if (d() && !d()->reportFileName().isEmpty())
        return QUrl::fromLocalFile(QFileInfo(d()->reportFileName()).absolutePath());
    if (d() && !d()->currentReportsDir().isEmpty())
        return QUrl::fromLocalFile(d()->currentReportsDir());
    return QUrl::fromLocalFile(QDir::currentPath());
}

void QuickReportDesigner::preview()
{
    if (!d() || d()->isBusy())
        return;
    d()->clearSelection();
    cancelInsert();
    d()->previewReport(PreviewBarsUserSetting, false);
    d()->dataManager()->setDesignTime(true);
}

bool QuickReportDesigner::exportToPdf(const QUrl& fileUrl)
{
    if (!d())
        return false;
    QString fileName = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    bool result = d()->printToPDF(fileName);
    d()->dataManager()->setDesignTime(true);
    return result;
}

void QuickReportDesigner::print()
{
    if (!d())
        return;
    d()->printReport();
    d()->dataManager()->setDesignTime(true);
}

// ----- pages ---------------------------------------------------------------

void QuickReportDesigner::addPage()
{
    if (!d())
        return;
    PageDesignIntf* page = d()->appendPage("page" + QString::number(d()->pageCount() + 1));
    connectPage(page);
    applyGrid();
    setLayoutEditMode(m_layoutEditMode);
    emit pagesChanged();
    setCurrentPageIndex(d()->pageCount() - 1);
    emit modifiedChanged();
}

bool QuickReportDesigner::deletePage(int index)
{
    if (!d() || d()->pageCount() <= 1 || index < 0 || index >= d()->pageCount())
        return false;
    PageDesignIntf* page = d()->pageAt(index);
    if (!d()->deletePage(page))
        return false;
    m_currentPageIndex = -1;
    emit currentPageChanged();
    emit pagesChanged();
    setCurrentPageIndex(qMax(0, index - 1));
    page->deleteLater();
    emit modifiedChanged();
    return true;
}

// ----- insertion -----------------------------------------------------------

void QuickReportDesigner::startInsert(const QString& itemType)
{
    PageDesignIntf* page = currentPage();
    if (!page)
        return;
    page->startInsertMode(itemType);
    m_insertItemType = itemType;
    emit insertModeChanged();
}

void QuickReportDesigner::cancelInsert()
{
    PageDesignIntf* page = currentPage();
    if (page && page->isItemInsertMode())
        page->startEditMode();
    if (!m_insertItemType.isEmpty()) {
        m_insertItemType.clear();
        emit insertModeChanged();
    }
}

void QuickReportDesigner::onItemInserted()
{
    m_insertItemType.clear();
    emit insertModeChanged();
    emit modifiedChanged();
}

void QuickReportDesigner::onInsertCanceled()
{
    m_insertItemType.clear();
    emit insertModeChanged();
}

void QuickReportDesigner::addBand(int bandType)
{
    PageDesignIntf* page = currentPage();
    if (!page)
        return;
    page->addBand(static_cast<BandDesignIntf::BandsType>(bandType));
    m_insertItemType.clear();
    emit insertModeChanged();
    emit selectionChanged();
    emit modifiedChanged();
}

// ----- editing -------------------------------------------------------------

#define WITH_PAGE(call)                                                                            \
    do {                                                                                           \
        if (PageDesignIntf* page = currentPage()) {                                                \
            page->call;                                                                            \
        }                                                                                          \
    } while (0)

void QuickReportDesigner::undo()
{
    WITH_PAGE(undo());
    onHistoryChanged();
}
void QuickReportDesigner::redo()
{
    WITH_PAGE(redo());
    onHistoryChanged();
}
void QuickReportDesigner::copy() { WITH_PAGE(copy()); }
void QuickReportDesigner::cut()
{
    WITH_PAGE(cut());
    onHistoryChanged();
}
void QuickReportDesigner::paste()
{
    WITH_PAGE(paste());
    onHistoryChanged();
}
void QuickReportDesigner::deleteSelected()
{
    WITH_PAGE(deleteSelected());
    onHistoryChanged();
}
void QuickReportDesigner::bringToFront() { WITH_PAGE(bringToFront()); }
void QuickReportDesigner::sendToBack() { WITH_PAGE(sendToBack()); }
void QuickReportDesigner::sameWidth() { WITH_PAGE(sameWidth()); }
void QuickReportDesigner::sameHeight() { WITH_PAGE(sameHeight()); }
void QuickReportDesigner::addHLayout()
{
    WITH_PAGE(addHLayout());
    onHistoryChanged();
}
void QuickReportDesigner::addVLayout()
{
    WITH_PAGE(addVLayout());
    onHistoryChanged();
}
void QuickReportDesigner::lockSelected()
{
    WITH_PAGE(lockSelectedItems());
    m_propertyModel->refresh();
}
void QuickReportDesigner::unlockSelected()
{
    WITH_PAGE(unlockSelectedItems());
    m_propertyModel->refresh();
}
void QuickReportDesigner::selectOneLevel() { WITH_PAGE(selectOneLevelItems()); }

void QuickReportDesigner::align(const QString& how)
{
    PageDesignIntf* page = currentPage();
    if (!page)
        return;
    if (how == "left")
        page->alignToLeft();
    else if (how == "right")
        page->alignToRigth();
    else if (how == "top")
        page->alignToTop();
    else if (how == "bottom")
        page->alignToBottom();
    else if (how == "hcenter")
        page->alignToHCenter();
    else if (how == "vcenter")
        page->alignToVCenter();
}

void QuickReportDesigner::setBorders(int borders)
{
    WITH_PAGE(setBorders(BaseDesignIntf::BorderLines(borders)));
    m_propertyModel->refresh();
}

void QuickReportDesigner::setTextAlignment(bool horizontal, int alignment)
{
    WITH_PAGE(changeSelectedGrpoupTextAlignPropperty(horizontal, Qt::AlignmentFlag(alignment)));
    m_propertyModel->refresh();
}

void QuickReportDesigner::setFontStyle(const QString& style, bool on)
{
    PageDesignIntf* page = currentPage();
    if (!page)
        return;
    for (GraphicsItem* graphicsItem : page->selectedItems()) {
        BaseDesignIntf* item = dynamic_cast<BaseDesignIntf*>(graphicsItem);
        if (!item || item->metaObject()->indexOfProperty("font") < 0)
            continue;
        QFont font = item->property("font").value<QFont>();
        if (style == "bold")
            font.setBold(on);
        else if (style == "italic")
            font.setItalic(on);
        else if (style == "underline")
            font.setUnderline(on);
        item->setProperty("font", font);
    }
    m_propertyModel->refresh();
}

void QuickReportDesigner::selectObject(QObject* object, bool addToSelection)
{
    PageDesignIntf* page = currentPage();
    BaseDesignIntf* item = qobject_cast<BaseDesignIntf*>(object);
    if (!page || !item)
        return;
    if (!addToSelection)
        page->clearSelection();
    item->setSelected(true);
}

QString QuickReportDesigner::itemContent(QObject* item) const
{
    return item ? item->property("content").toString() : QString();
}

void QuickReportDesigner::setItemContent(QObject* item, const QString& content)
{
    if (!item)
        return;
    item->setProperty("content", content);
    m_propertyModel->refresh();
    emit modifiedChanged();
}

void QuickReportDesigner::onPageSelectionChanged()
{
    if (sender() != currentPage())
        return;
    updateSelectionModels();
    emit selectionChanged();
}

void QuickReportDesigner::updateSelectionModels()
{
    QList<QObject*> objects;
    PageDesignIntf* page = currentPage();
    if (page) {
        for (GraphicsItem* item : page->selectedItems()) {
            BaseDesignIntf* baseItem = dynamic_cast<BaseDesignIntf*>(item);
            if (baseItem)
                objects.append(baseItem);
        }
        if (objects.isEmpty() && page->pageItem())
            objects.append(page->pageItem());
    }
    m_propertyModel->setObjects(objects);
}

void QuickReportDesigner::onHistoryChanged()
{
    emit historyChanged();
    emit modifiedChanged();
}

void QuickReportDesigner::onPropertyChanged()
{
    if (!m_refreshTimer.isActive())
        m_refreshTimer.start();
}

void QuickReportDesigner::onItemEditorRequested(BaseDesignIntf* item)
{
    if (!item)
        return;
    QString className = QString::fromLatin1(item->metaObject()->className());
    QString kind = "generic";
    if (className.endsWith("TextItem"))
        kind = "text";
    else if (className.endsWith("ImageItem"))
        kind = "image";
    else if (className.endsWith("SVGItem"))
        kind = "svg";
    else if (className.endsWith("ChartItem"))
        kind = "chart";
    emit editItemRequested(item, kind);
}

// ----- data sources ----------------------------------------------------------

QVariantMap QuickReportDesigner::connectionInfo(const QString& name) const
{
    QVariantMap result;
    if (!dataManager())
        return result;
    ConnectionDesc* connection
        = dataManager()->connectionByName(ConnectionDesc::connectionNameForReport(name));
    if (!connection)
        return result;
    result.insert("name", ConnectionDesc::connectionNameForUser(connection->name()));
    result.insert("driver", connection->driver());
    result.insert("databaseName", connection->databaseName());
    result.insert("userName", connection->userName());
    result.insert("password", connection->password());
    result.insert("host", connection->host());
    result.insert("port", connection->port());
    result.insert("autoconnect", connection->autoconnect());
    result.insert("keepDBCredentials", connection->keepDBCredentials());
    result.insert("isDefault", connection->name() == QSqlDatabase::defaultConnection);
    return result;
}

static void fillConnection(ConnectionDesc* connection, const QVariantMap& info)
{
    QString name = info.value("isDefault").toBool() ? QString(QSqlDatabase::defaultConnection)
                                                    : info.value("name").toString();
    connection->setName(ConnectionDesc::connectionNameForReport(name));
    connection->setDriver(info.value("driver").toString());
    connection->setDatabaseName(info.value("databaseName").toString());
    connection->setUserName(info.value("userName").toString());
    connection->setPassword(info.value("password").toString());
    connection->setHost(info.value("host").toString());
    if (!info.value("port").toString().isEmpty())
        connection->setPort(info.value("port").toString());
    connection->setAutoconnect(info.value("autoconnect").toBool());
    connection->setKeepDBCredentials(info.value("keepDBCredentials", true).toBool());
}

bool QuickReportDesigner::checkConnection(const QVariantMap& info)
{
    if (!dataManager())
        return false;
    ConnectionDesc connection;
    fillConnection(&connection, info);
    bool result = dataManager()->checkConnectionDesc(&connection);
    if (!result)
        setLastError(dataManager()->lastError());
    return result;
}

bool QuickReportDesigner::saveConnection(const QVariantMap& info, const QString& oldName)
{
    DataSourceManager* dm = dataManager();
    if (!dm)
        return false;
    QString name = info.value("isDefault").toBool() ? QString(QSqlDatabase::defaultConnection)
                                                    : info.value("name").toString();
    if (name.isEmpty()) {
        setLastError(tr("Connection Name is empty"));
        return false;
    }
    try {
        if (oldName.isEmpty()) {
            if (QSqlDatabase::connectionNames().contains(name) || dm->connectionByName(name)) {
                setLastError(tr("Connection with name %1 already exists!").arg(name));
                return false;
            }
            if (info.value("autoconnect").toBool() && !checkConnection(info))
                return false;
            ConnectionDesc* connection = new ConnectionDesc();
            fillConnection(connection, info);
            dm->addConnectionDesc(connection);
        } else {
            ConnectionDesc* connection
                = dm->connectionByName(ConnectionDesc::connectionNameForReport(oldName));
            if (!connection)
                return false;
            fillConnection(connection, info);
            if (connection->autoconnect())
                dm->connectConnection(connection->name());
        }
    } catch (ReportError& error) {
        setLastError(error.what());
        return false;
    }
    refreshData();
    return true;
}

bool QuickReportDesigner::deleteConnection(const QString& name)
{
    if (!dataManager())
        return false;
    dataManager()->removeConnection(ConnectionDesc::connectionNameForReport(name));
    refreshData();
    return true;
}

bool QuickReportDesigner::toggleConnection(const QString& name)
{
    DataSourceManager* dm = dataManager();
    if (!dm)
        return false;
    QString reportName = ConnectionDesc::connectionNameForReport(name);
    bool result = true;
    if (dm->isConnectionConnected(reportName)) {
        dm->disconnectConnection(reportName);
    } else {
        result = dm->connectConnection(reportName);
        if (!result)
            setLastError(dm->lastError());
    }
    dm->updateDatasourceModel();
    refreshData();
    return result;
}

QVariantMap QuickReportDesigner::datasourceInfo(const QString& name) const
{
    QVariantMap result;
    DataSourceManager* dm = dataManager();
    if (!dm || !dm->containsDatasource(name))
        return result;
    result.insert("name", name);
    if (dm->isQuery(name)) {
        result.insert("type", "query");
        result.insert("sql", dm->queryText(name));
        result.insert("connection",
                      ConnectionDesc::connectionNameForUser(dm->connectionName(name)));
    } else if (dm->isSubQuery(name)) {
        SubQueryDesc* desc = dm->subQueryByName(name);
        result.insert("type", "subquery");
        result.insert("sql", dm->queryText(name));
        result.insert("connection",
                      ConnectionDesc::connectionNameForUser(dm->connectionName(name)));
        if (desc)
            result.insert("master", desc->master());
    } else if (dm->isCSV(name)) {
        CSVDesc* desc = dm->csvByName(name);
        result.insert("type", "csv");
        if (desc) {
            result.insert("csv", desc->csvText());
            result.insert("separator", desc->separator());
            result.insert("firstRowIsHeader", desc->firstRowIsHeader());
        }
    } else if (dm->isProxy(name)) {
        result.insert("type", "proxy");
    } else {
        result.insert("type", "external");
    }
    return result;
}

bool QuickReportDesigner::saveDatasource(const QVariantMap& info, const QString& oldName)
{
    DataSourceManager* dm = dataManager();
    if (!dm)
        return false;
    QString name = info.value("name").toString();
    QString type = info.value("type", "query").toString();
    if (name.isEmpty()) {
        setLastError(tr("Datasource Name is empty!"));
        return false;
    }
    if (name != oldName && dm->containsDatasource(name)) {
        setLastError(tr("Datasource with name %1 already exists!").arg(name));
        return false;
    }
    try {
        if (!oldName.isEmpty())
            dm->removeDatasource(oldName);
        QString connection
            = ConnectionDesc::connectionNameForReport(info.value("connection").toString());
        if (type == "subquery")
            dm->addSubQuery(name, info.value("sql").toString(), connection,
                            info.value("master").toString());
        else if (type == "csv")
            dm->addCSV(name, info.value("csv").toString(), info.value("separator", ";").toString(),
                       info.value("firstRowIsHeader").toBool());
        else
            dm->addQuery(name, info.value("sql").toString(), connection);
    } catch (ReportError& error) {
        setLastError(error.what());
        return false;
    }
    refreshData();
    return true;
}

bool QuickReportDesigner::deleteDatasource(const QString& name)
{
    if (!dataManager())
        return false;
    dataManager()->removeDatasource(name);
    refreshData();
    return true;
}

QVariantMap QuickReportDesigner::variableInfo(const QString& name) const
{
    QVariantMap result;
    DataSourceManager* dm = dataManager();
    if (!dm || !dm->containsVariable(name))
        return result;
    result.insert("name", name);
    result.insert("value", dm->variable(name).toString());
    result.insert("mandatory", dm->variableIsMandatory(name));
    QMetaEnum e = QMetaEnum::fromType<Enums::VariableDataType>();
    result.insert("type", QString::fromLatin1(e.valueToKey(dm->variableDataType(name))));
    return result;
}

bool QuickReportDesigner::saveVariable(const QVariantMap& info, const QString& oldName)
{
    DataSourceManager* dm = dataManager();
    if (!dm)
        return false;
    QString name = info.value("name").toString();
    if (name.isEmpty()) {
        setLastError(tr("Variable name is empty"));
        return false;
    }
    QMetaEnum e = QMetaEnum::fromType<Enums::VariableDataType>();
    VariableDataType dataType
        = VariableDataType(e.keyToValue(info.value("type", "String").toString().toLatin1()));
    QVariant value = info.value("value");
    switch (dataType) {
    case Enums::Bool:
        value = QVariant(value.toString().compare("true", Qt::CaseInsensitive) == 0
                         || value.toString() == "1");
        break;
    case Enums::Int:
        value = value.toString().toInt();
        break;
    case Enums::Real:
        value = value.toString().toDouble();
        break;
    case Enums::Date:
        value = QDate::fromString(value.toString(), Qt::ISODate);
        break;
    case Enums::Time:
        value = QTime::fromString(value.toString(), Qt::ISODate);
        break;
    case Enums::DateTime:
        value = QDateTime::fromString(value.toString(), Qt::ISODate);
        break;
    default:
        value = value.toString();
        break;
    }
    try {
        if (!oldName.isEmpty()) {
            if (oldName == name)
                dm->changeVariable(oldName, value);
            else {
                dm->deleteVariable(oldName);
                dm->addVariable(name, value, VarDesc::Report);
            }
        } else {
            if (dm->containsVariable(name)) {
                setLastError(tr("Variable with name %1 already exists").arg(name));
                return false;
            }
            dm->addVariable(name, value, VarDesc::Report);
        }
        dm->setVarableMandatory(name, info.value("mandatory").toBool());
        dm->setVariableDataType(name, dataType);
    } catch (ReportError& error) {
        setLastError(error.what());
        return false;
    }
    refreshData();
    return true;
}

bool QuickReportDesigner::deleteVariable(const QString& name)
{
    if (!dataManager())
        return false;
    dataManager()->deleteVariable(name);
    refreshData();
    return true;
}

void QuickReportDesigner::refreshData()
{
    m_dataModel->rebuild();
    emit dataChanged();
    emit modifiedChanged();
}

} // namespace LimeReport
