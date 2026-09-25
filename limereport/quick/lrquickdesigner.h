#ifndef LRQUICKDESIGNER_H
#define LRQUICKDESIGNER_H

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

namespace LimeReport {

class ReportEngine;
class ReportEnginePrivate;
class PageDesignIntf;
class BaseDesignIntf;
class DataSourceManager;
class QuickPropertyModel;
class QuickDataBrowserModel;
class QuickObjectTreeModel;

/*
 * Backend of the QML ReportDesigner component: exposes pages, editing
 * commands, the property / data / object models and file handling of a
 * ReportEngine to Qt Quick.
 */
class QuickReportDesigner : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ReportDesignerController)
    Q_PROPERTY(QObject* engine READ engineObject WRITE setEngineObject NOTIFY engineChanged)
    Q_PROPERTY(QStringList pageNames READ pageNames NOTIFY pagesChanged)
    Q_PROPERTY(int currentPageIndex READ currentPageIndex WRITE setCurrentPageIndex NOTIFY currentPageChanged)
    Q_PROPERTY(QObject* scene READ scene NOTIFY currentPageChanged)
    Q_PROPERTY(QObject* propertyModel READ propertyModel CONSTANT)
    Q_PROPERTY(QObject* dataModel READ dataModel CONSTANT)
    Q_PROPERTY(QObject* objectTreeModel READ objectTreeModel CONSTANT)
    Q_PROPERTY(QString reportFileName READ reportFileName NOTIFY reportChanged)
    Q_PROPERTY(QString reportName READ reportName NOTIFY reportChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
    Q_PROPERTY(QObject* selectedObject READ selectedObject NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList itemTypes READ itemTypes CONSTANT)
    Q_PROPERTY(QVariantList bandTypes READ bandTypes NOTIFY selectionChanged)
    Q_PROPERTY(QString insertItemType READ insertItemType NOTIFY insertModeChanged)
    Q_PROPERTY(QString script READ script WRITE setScript NOTIFY scriptChanged)
    Q_PROPERTY(bool magneticMovement READ magneticMovement WRITE setMagneticMovement NOTIFY settingsChanged)
    Q_PROPERTY(bool useGrid READ useGrid WRITE setUseGrid NOTIFY settingsChanged)
    Q_PROPERTY(int gridStep READ gridStep WRITE setGridStep NOTIFY settingsChanged)
    Q_PROPERTY(bool layoutEditMode READ layoutEditMode WRITE setLayoutEditMode NOTIFY settingsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QStringList sqlDrivers READ sqlDrivers CONSTANT)
    Q_PROPERTY(QStringList connectionNames READ connectionNames NOTIFY dataChanged)
    Q_PROPERTY(QStringList datasourceNames READ datasourceNames NOTIFY dataChanged)
    Q_PROPERTY(QStringList variableTypes READ variableTypes CONSTANT)
public:
    explicit QuickReportDesigner(QObject* parent = nullptr);
    ~QuickReportDesigner();

    QObject* engineObject() const;
    void setEngineObject(QObject* engine);

    QStringList pageNames() const;
    int currentPageIndex() const { return m_currentPageIndex; }
    void setCurrentPageIndex(int index);
    QObject* scene() const;
    PageDesignIntf* currentPage() const;

    QObject* propertyModel() const;
    QObject* dataModel() const;
    QObject* objectTreeModel() const;

    QString reportFileName() const;
    QString reportName() const;
    bool modified() const;
    bool canUndo() const;
    bool canRedo() const;
    int selectionCount() const;
    QObject* selectedObject() const;
    QVariantList itemTypes() const;
    QVariantList bandTypes() const;
    QString insertItemType() const { return m_insertItemType; }
    QString script() const;
    void setScript(const QString& script);
    bool magneticMovement() const { return m_magneticMovement; }
    void setMagneticMovement(bool value);
    bool useGrid() const { return m_useGrid; }
    void setUseGrid(bool value);
    int gridStep() const { return m_gridStep; }
    void setGridStep(int value);
    bool layoutEditMode() const { return m_layoutEditMode; }
    void setLayoutEditMode(bool value);
    QString lastError() const { return m_lastError; }
    QStringList sqlDrivers() const;
    QStringList connectionNames() const;
    QStringList datasourceNames() const;
    QStringList variableTypes() const;

    // report / file handling
    Q_INVOKABLE void newReport();
    Q_INVOKABLE bool openReport(const QUrl& fileUrl);
    // Returns false when the report has no file name yet (use saveReportAs).
    Q_INVOKABLE bool saveReport();
    Q_INVOKABLE bool saveReportAs(const QUrl& fileUrl);
    Q_INVOKABLE bool hasFileName() const;
    Q_INVOKABLE void preview();
    Q_INVOKABLE bool exportToPdf(const QUrl& fileUrl);
    Q_INVOKABLE void print();
    Q_INVOKABLE QUrl reportFolder() const;

    // pages
    Q_INVOKABLE void addPage();
    Q_INVOKABLE bool deletePage(int index);

    // insertion
    Q_INVOKABLE void startInsert(const QString& itemType);
    Q_INVOKABLE void cancelInsert();
    Q_INVOKABLE void addBand(int bandType);

    // editing
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void deleteSelected();
    Q_INVOKABLE void bringToFront();
    Q_INVOKABLE void sendToBack();
    Q_INVOKABLE void align(const QString& how);
    Q_INVOKABLE void sameWidth();
    Q_INVOKABLE void sameHeight();
    Q_INVOKABLE void addHLayout();
    Q_INVOKABLE void addVLayout();
    Q_INVOKABLE void lockSelected();
    Q_INVOKABLE void unlockSelected();
    Q_INVOKABLE void selectOneLevel();
    Q_INVOKABLE void setBorders(int borders);
    Q_INVOKABLE void setTextAlignment(bool horizontal, int alignment);
    Q_INVOKABLE void setFontStyle(const QString& style, bool on);
    Q_INVOKABLE void selectObject(QObject* object, bool addToSelection = false);
    Q_INVOKABLE QString itemContent(QObject* item) const;
    Q_INVOKABLE void setItemContent(QObject* item, const QString& content);
    Q_INVOKABLE QStringList fieldNames(const QString& datasourceName) const;

    // data sources
    Q_INVOKABLE QVariantMap connectionInfo(const QString& name) const;
    Q_INVOKABLE bool checkConnection(const QVariantMap& info);
    Q_INVOKABLE bool saveConnection(const QVariantMap& info, const QString& oldName = QString());
    Q_INVOKABLE bool deleteConnection(const QString& name);
    Q_INVOKABLE bool toggleConnection(const QString& name);
    Q_INVOKABLE QVariantMap datasourceInfo(const QString& name) const;
    Q_INVOKABLE bool saveDatasource(const QVariantMap& info, const QString& oldName = QString());
    Q_INVOKABLE bool deleteDatasource(const QString& name);
    Q_INVOKABLE QVariantMap variableInfo(const QString& name) const;
    Q_INVOKABLE bool saveVariable(const QVariantMap& info, const QString& oldName = QString());
    Q_INVOKABLE bool deleteVariable(const QString& name);
    Q_INVOKABLE void refreshData();

signals:
    void engineChanged();
    void pagesChanged();
    void currentPageChanged();
    void reportChanged();
    void modifiedChanged();
    void historyChanged();
    void selectionChanged();
    void insertModeChanged();
    void scriptChanged();
    void settingsChanged();
    void lastErrorChanged();
    void dataChanged();
    // An item asked for its editor (double click / "Edit" menu entry).
    // kind is one of "text", "image", "svg", "chart", "generic".
    void editItemRequested(QObject* item, const QString& kind);
    void message(int severity, const QString& title, const QString& text);

private slots:
    void onEngineDestroyed();
    void onReportLoaded();
    void onReportCleared();
    void onPageSelectionChanged();
    void onHistoryChanged();
    void onItemInserted();
    void onInsertCanceled();
    void onItemEditorRequested(LimeReport::BaseDesignIntf* item);
    void onPropertyChanged();

private:
    ReportEnginePrivate* d() const;
    DataSourceManager* dataManager() const;
    void connectPage(PageDesignIntf* page);
    void createStartPage();
    void applyGrid();
    void setLastError(const QString& error);
    void updateSelectionModels();

private:
    QPointer<ReportEngine> m_engine;
    int m_currentPageIndex;
    QuickPropertyModel* m_propertyModel;
    QuickDataBrowserModel* m_dataModel;
    QuickObjectTreeModel* m_objectTreeModel;
    QString m_insertItemType;
    bool m_magneticMovement;
    bool m_useGrid;
    int m_gridStep;
    bool m_layoutEditMode;
    QString m_lastError;
    QTimer m_refreshTimer;
};

} // namespace LimeReport

#endif // LRQUICKDESIGNER_H
