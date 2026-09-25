#ifndef LRQUICKPREVIEW_H
#define LRQUICKPREVIEW_H

#include "lrreportengine.h"

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace LimeReport {

class ReportEngine;
class ReportEnginePrivate;
class PageDesignIntf;

/*
 * Backend of the QML ReportPreview component. Holds the rendered pages of a
 * report in a scene that a ReportSceneView displays.
 */
class QuickReportPreview: public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ReportPreviewController)
    Q_PROPERTY(QObject* engine READ engineObject WRITE setEngineObject NOTIFY engineChanged)
    Q_PROPERTY(QObject* scene READ scene NOTIFY pagesChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pagesChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QStringList errorMessages READ errorMessages NOTIFY pagesChanged)
    Q_PROPERTY(QString title READ title NOTIFY engineChanged)
    Q_PROPERTY(QColor pageBackgroundColor READ pageBackgroundColor NOTIFY engineChanged)
    Q_PROPERTY(bool resultEditable READ resultEditable NOTIFY engineChanged)
    Q_PROPERTY(bool printVisible READ printVisible NOTIFY engineChanged)
    Q_PROPERTY(bool printToPdfVisible READ printToPdfVisible NOTIFY engineChanged)
    Q_PROPERTY(bool saveToFileVisible READ saveToFileVisible NOTIFY engineChanged)
    Q_PROPERTY(int scaleType READ scaleType NOTIFY engineChanged)
    Q_PROPERTY(int scalePercent READ scalePercent NOTIFY engineChanged)
    Q_PROPERTY(QVariantList exporters READ exporters CONSTANT)
public:
    explicit QuickReportPreview(QObject* parent = nullptr);
    ~QuickReportPreview();

    QObject* engineObject() const;
    void setEngineObject(QObject* engine);
    ReportEngine* engine() const { return m_engine; }

    QObject* scene() const;
    int pageCount() const { return m_pages.count(); }
    int currentPage() const { return m_currentPage; }
    void setCurrentPage(int page);
    bool busy() const { return m_busy; }
    bool editMode() const { return m_editMode; }
    void setEditMode(bool value);
    QString lastError() const { return m_lastError; }
    QStringList errorMessages() const { return m_errorMessages; }
    QString title() const;
    QColor pageBackgroundColor() const;
    bool resultEditable() const;
    bool printVisible() const;
    bool printToPdfVisible() const;
    bool saveToFileVisible() const;
    int scaleType() const;
    int scalePercent() const;
    QVariantList exporters() const;

    void setPages(ReportPages pages);
    ReportPages pages() const { return m_pages; }

    // Renders the report of the attached engine.
    Q_INVOKABLE bool render();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void reloadPreview() { render(); }
    Q_INVOKABLE QRectF pageRect(int page) const;
    Q_INVOKABLE int pageAt(qreal sceneX, qreal sceneY) const;
    Q_INVOKABLE bool exportToPdf(const QUrl& fileUrl);
    Q_INVOKABLE bool exportTo(const QString& exporterName, const QUrl& fileUrl);
    Q_INVOKABLE bool print();
    Q_INVOKABLE bool savePages(const QUrl& fileUrl);
    Q_INVOKABLE bool loadPages(const QUrl& fileUrl);
    Q_INVOKABLE QString defaultFileName(const QString& extension) const;
    // Edit mode helpers (the result is editable when the engine allows it).
    Q_INVOKABLE void startInsertTextItem();
    Q_INVOKABLE void selectionMode();
    Q_INVOKABLE void deleteSelectedItems();

signals:
    void engineChanged();
    void pagesChanged();
    void currentPageChanged();
    void busyChanged();
    void editModeChanged();
    void lastErrorChanged();
    void renderPageFinished(int page);

private slots:
    void onEngineDestroyed();
    void onSelectionChanged();

private:
    ReportEnginePrivate* d() const;
    void setLastError(const QString& error);
    static QString localFile(const QUrl& url);

private:
    QPointer<ReportEngine> m_engine;
    PageDesignIntf* m_scene;
    ReportPages m_pages;
    int m_currentPage;
    bool m_busy;
    bool m_editMode;
    QString m_lastError;
    QStringList m_errorMessages;
};

} // namespace LimeReport

#endif // LRQUICKPREVIEW_H
