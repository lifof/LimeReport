#ifndef DEMOBACKEND_H
#define DEMOBACKEND_H

#include <QObject>
#include <QUrl>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtQml/qqmlregistration.h>
#include <LimeReport>
#include <LRCallbackDS>

// Owns the report engine of the demo and feeds it with external data
// (Qt models and callback datasources).
class DemoBackend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QObject* engine READ engine CONSTANT)
    Q_PROPERTY(QStringList reports READ reports CONSTANT)
    Q_PROPERTY(QUrl reportsFolder READ reportsFolder CONSTANT)
public:
    explicit DemoBackend(QObject* parent = nullptr);
    ~DemoBackend();

    QObject* engine() const { return m_report; }
    QStringList reports() const;
    QUrl reportsFolder() const;

    Q_INVOKABLE bool load(const QString& reportName);
    Q_INVOKABLE void setVariable(const QString& name, const QString& value);
    Q_INVOKABLE void designReport();
    Q_INVOKABLE void previewReport();

private:
    void slotGetCallbackData(LimeReport::CallbackInfo info, QVariant& data);
    void slotChangePos(const LimeReport::CallbackInfo::ChangePosType& type, bool& result);
    void slotGetCallbackChildData(LimeReport::CallbackInfo info, QVariant& data);
    void slotChangeChildPos(const LimeReport::CallbackInfo::ChangePosType& type, bool& result);
    void slotOneSlotDS(LimeReport::CallbackInfo info, QVariant& data);

private:
    void prepareData(QSqlQuery* ds, LimeReport::CallbackInfo info, QVariant& data);
    QString reportsDir() const;

private:
    LimeReport::ReportEngine* m_report;
    QSqlDatabase m_db;
    QSqlQuery* m_customers;
    QSqlQuery* m_orders;
};

#endif // DEMOBACKEND_H
