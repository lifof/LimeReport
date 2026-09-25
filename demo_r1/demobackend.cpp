#include "demobackend.h"

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QStringListModel>
#include <QtSql/QSqlQueryModel>
#include <QtSql/QSqlRecord>

DemoBackend::DemoBackend(QObject* parent):
    QObject(parent),
    m_report(new LimeReport::ReportEngine(this)),
    m_customers(nullptr),
    m_orders(nullptr)
{
    // Reports refer to "./demo_reports/northwind.db"
    QDir::setCurrent(QFileInfo(reportsDir()).absolutePath());

    QFile dbFile(reportsDir() + "/northwind.db");
    if (dbFile.exists()) {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setDatabaseName(dbFile.fileName());
        if (m_db.open()) {
            QSqlQueryModel* customersModel = new QSqlQueryModel();
            customersModel->setQuery("select * from customers", m_db);
            m_report->dataManager()->addModel("external_customers_data", customersModel, true);
            QSqlQueryModel* ordersModel = new QSqlQueryModel();
            ordersModel->setQuery("Select * from orders", m_db);
            m_report->dataManager()->addModel("external_orders_data", ordersModel, true);
            m_customers = new QSqlQuery("Select * from customers limit 10", m_db);
            m_customers->first();
            m_orders = new QSqlQuery(m_db);
            m_orders->prepare("Select * from orders where CustomerID = :id");
            int index = m_customers->record().indexOf("CustomerID");
            m_orders->bindValue(":id", m_customers->value(index));
            m_orders->exec();
        }
    }

    using LimeReport::CallbackInfo;
    LimeReport::ICallbackDatasource* callbackDatasource
        = m_report->dataManager()->createCallbackDatasource("master");
    connect(callbackDatasource, &LimeReport::ICallbackDatasource::getCallbackData, this,
            [this](const CallbackInfo& info, QVariant& data) { slotGetCallbackData(info, data); });
    connect(callbackDatasource, &LimeReport::ICallbackDatasource::changePos, this,
            [this](const CallbackInfo::ChangePosType& type, bool& result) {
                slotChangePos(type, result);
            });

    callbackDatasource = m_report->dataManager()->createCallbackDatasource("detail");
    connect(
        callbackDatasource, &LimeReport::ICallbackDatasource::getCallbackData, this,
        [this](const CallbackInfo& info, QVariant& data) { slotGetCallbackChildData(info, data); });
    connect(callbackDatasource, &LimeReport::ICallbackDatasource::changePos, this,
            [this](const CallbackInfo::ChangePosType& type, bool& result) {
                slotChangeChildPos(type, result);
            });

    callbackDatasource = m_report->dataManager()->createCallbackDatasource("oneSlotDS");
    connect(callbackDatasource, &LimeReport::ICallbackDatasource::getCallbackData, this,
            [this](const CallbackInfo& info, QVariant& data) { slotOneSlotDS(info, data); });

    QStringListModel* stringListModel = new QStringListModel();
    stringListModel->setStringList(QStringList() << "value1"
                                                 << "value2"
                                                 << "value3");
    m_report->dataManager()->addModel("string_list", stringListModel, true);
}

DemoBackend::~DemoBackend()
{
    delete m_customers;
    delete m_orders;
}

QString DemoBackend::reportsDir() const
{
    QString dir = QCoreApplication::applicationDirPath() + "/demo_reports";
    if (QDir(dir).exists())
        return dir;
    return QStringLiteral(DEMO_REPORTS_DIR);
}

QStringList DemoBackend::reports() const
{
    return QDir(reportsDir()).entryList(QStringList() << "*.lrxml", QDir::Files, QDir::Name);
}

QUrl DemoBackend::reportsFolder() const { return QUrl::fromLocalFile(reportsDir()); }

bool DemoBackend::load(const QString& reportName)
{
    return m_report->loadFromFile(reportsDir() + "/" + reportName);
}

void DemoBackend::setVariable(const QString& name, const QString& value)
{
    m_report->dataManager()->clearUserVariables();
    if (!name.isEmpty() && !value.isEmpty())
        m_report->dataManager()->setReportVariable(name, value);
}

void DemoBackend::designReport()
{
    m_report->setShowDesignerModal(false);
    m_report->designReport();
}

void DemoBackend::previewReport()
{
    m_report->setShowPreviewModal(false);
    m_report->previewReport();
}

void DemoBackend::prepareData(QSqlQuery* ds, LimeReport::CallbackInfo info, QVariant& data)
{
    switch (info.dataType) {
    case LimeReport::CallbackInfo::ColumnCount:
        data = ds->record().count();
        break;
    case LimeReport::CallbackInfo::IsEmpty:
        data = !ds->first();
        break;
    case LimeReport::CallbackInfo::HasNext:
        data = ds->next();
        ds->previous();
        break;
    case LimeReport::CallbackInfo::ColumnHeaderData:
        if (info.index < ds->record().count())
            data = ds->record().fieldName(info.index);
        break;
    case LimeReport::CallbackInfo::ColumnData:
        data = ds->value(ds->record().indexOf(info.columnName));
        break;
    default:
        break;
    }
}

void DemoBackend::slotGetCallbackData(LimeReport::CallbackInfo info, QVariant& data)
{
    if (!m_customers)
        return;
    prepareData(m_customers, info, data);
}

void DemoBackend::slotChangePos(const LimeReport::CallbackInfo::ChangePosType& type, bool& result)
{
    QSqlQuery* ds = m_customers;
    if (!ds)
        return;
    if (type == LimeReport::CallbackInfo::First)
        result = ds->first();
    else
        result = ds->next();
    if (result) {
        m_orders->bindValue(":id", m_customers->value(m_customers->record().indexOf("CustomerID")));
        m_orders->exec();
    }
}

void DemoBackend::slotGetCallbackChildData(LimeReport::CallbackInfo info, QVariant& data)
{
    if (!m_orders)
        return;
    prepareData(m_orders, info, data);
}

void DemoBackend::slotChangeChildPos(const LimeReport::CallbackInfo::ChangePosType& type,
                                     bool& result)
{
    QSqlQuery* ds = m_orders;
    if (!ds)
        return;
    if (type == LimeReport::CallbackInfo::First)
        result = ds->first();
    else
        result = ds->next();
}

void DemoBackend::slotOneSlotDS(LimeReport::CallbackInfo info, QVariant& data)
{
    QStringList columns;
    columns << "Name"
            << "Value"
            << "Image";
    switch (info.dataType) {
    case LimeReport::CallbackInfo::RowCount:
        data = 4;
        break;
    case LimeReport::CallbackInfo::ColumnCount:
        data = columns.size();
        break;
    case LimeReport::CallbackInfo::ColumnHeaderData:
        data = columns.at(info.index);
        break;
    case LimeReport::CallbackInfo::ColumnData:
        if (info.columnName == "Image")
            data = QImage(":/report/images/logo32");
        else
            data = info.columnName + " " + QString::number(info.index);
        break;
    default:
        break;
    }
}
