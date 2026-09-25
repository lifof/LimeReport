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
#ifndef LRREPORTDESIGNINTF_H
#define LRREPORTDESIGNINTF_H

#include <QObject>
#include <QSettings>
#include <QIcon>
#include <QColor>
#include <QFont>
#include <QImage>
#include <QUrl>
#include <QLocale>

#include "lrglobal.h"
#include "lrdatasourcemanagerintf.h"
#include "lrscriptenginemanagerintf.h"
#include "lrpreparedpagesintf.h"

namespace LimeReport {

class GraphicsScene;

class LIMEREPORT_EXPORT PrintRange{
public:
    enum RangeType { AllPages, PageRange };
    int fromPage() const { return m_fromPage;}
    int toPage() const { return m_toPage;}
    RangeType rangeType() const { return m_rangeType;}
    PrintRange(RangeType rangeType = AllPages, int fromPage = 0, int toPage = 0)
        : m_rangeType(rangeType), m_fromPage(fromPage), m_toPage(toPage){}
    void setRangeType(RangeType rangeType){ m_rangeType=rangeType;}
    void setFromPage(int fromPage){ m_fromPage = fromPage;}
    void setToPage(int toPage){ m_toPage = toPage;}
    bool contains(int page) const { return m_rangeType == AllPages || (page >= m_fromPage && page <= m_toPage); }
private:
    RangeType m_rangeType;
    int m_fromPage;
    int m_toPage;
};

class LIMEREPORT_EXPORT ItemGeometry{
public:
    enum Type{Millimeters, Pixels};
    ItemGeometry(qreal x, qreal y, qreal width, qreal height, Qt::Alignment anchor, Type type = Millimeters)
        :m_x(x), m_y(y), m_width(width), m_height(height), m_type(type), m_anchor(anchor){}
    ItemGeometry(): m_x(0), m_y(0), m_width(0), m_height(0), m_type(Millimeters){}

    qreal x() const;
    void setX(const qreal &x);

    qreal y() const;
    void setY(const qreal &y);

    qreal width() const;
    void setWidth(const qreal &width);

    qreal height() const;
    void setHeight(const qreal &height);

    Type type() const;
    void setType(const Type &type);

    Qt::Alignment anchor() const;
    void setAnchor(const Qt::Alignment &anchor);

private:
    qreal m_x;
    qreal m_y;
    qreal m_width;
    qreal m_height;
    Type m_type;
    Qt::Alignment m_anchor;
};

class LIMEREPORT_EXPORT WatermarkSetting{
public:
    WatermarkSetting(const QString& text, const ItemGeometry& geometry, const QFont& font)
        : m_text(text), m_font(font), m_opacity(50), m_geometry(geometry), m_color(QColor(Qt::black)){}
    WatermarkSetting(): m_font(QFont()), m_opacity(50), m_geometry(ItemGeometry()){}
    QString text() const;
    void setText(const QString &text);

    QFont font() const;
    void setFont(const QFont &font);

    int opacity() const;
    void setOpacity(const int &opacity);

    ItemGeometry geometry() const;
    void setGeometry(const ItemGeometry &geometry);

    QColor color() const;
    void setColor(const QColor &color);

private:
    QString m_text;
    QFont   m_font;
    int   m_opacity;
    ItemGeometry m_geometry;
    QColor m_color;
};

class ItemBuilder{
    virtual void setProperty(QString name, QVariant value) = 0;
    virtual QVariant property(QString name) = 0;
    virtual void setGeometry(ItemGeometry geometry) = 0;
    virtual ItemGeometry geometry() = 0; 
};


class DataSourceManager;
class ReportEnginePrivate;
class PageDesignIntf;
class PageItemDesignIntf;
class PreparedPages;

typedef QList< QSharedPointer<PageItemDesignIntf> > ReportPages;

class LIMEREPORT_EXPORT ReportEngine : public QObject{
    Q_OBJECT
    friend class QuickReportPreview;
    friend class QuickReportDesigner;
public:
    static void setSettings(QSettings *value){m_settings=value;}
public:
    explicit ReportEngine(QObject *parent = 0);
    ~ReportEngine();
    // Renders the report and writes it as PDF (QPdfWriter, no printer subsystem).
    bool    printToPDF(const QString& fileName, const PrintRange& range = PrintRange());
    bool    printPagesToPDF(ReportPages pages, const QString& fileName, const PrintRange& range = PrintRange());
    // "Printing" hands a rendered PDF to the platform's default viewer/printer.
    Q_INVOKABLE bool printReport();
    void    printToFile(const QString& fileName);
    GraphicsScene* createPreviewScene(QObject *parent = 0);
    bool    exportReport(QString exporterName, const QString &fileName = "", const QMap<QString, QVariant>& params = QMap<QString, QVariant>());
    // Renders every page of the report into images at the given resolution.
    QList<QImage> renderToImages(qreal dpi = 96);
    // Opens the Qt Quick preview window. Blocks until it is closed when modal.
    Q_INVOKABLE void previewReport(LimeReport::PreviewHints hints = PreviewBarsUserSetting);
    // Opens the Qt Quick designer window.
    Q_INVOKABLE void designReport();
    void    setShowProgressDialog(bool value);
    bool    isShowProgressDialog();
    IDataSourceManager* dataManager();
    IScriptEngineManager* scriptManager();
    Q_INVOKABLE bool loadFromFile(const QString& fileName, bool autoLoadPreviewOnChange = false);
    // QML friendly variants taking file URLs.
    Q_INVOKABLE bool loadFromUrl(const QUrl& fileUrl, bool autoLoadPreviewOnChange = false);
    Q_INVOKABLE bool saveToUrl(const QUrl& fileUrl);
    Q_INVOKABLE bool printToPdfUrl(const QUrl& fileUrl);
    bool    loadFromByteArray(QByteArray *data);
    Q_INVOKABLE bool loadFromString(const QString& data);
    Q_INVOKABLE QString reportFileName();
    void    setReportFileName(const QString& fileName);
    Q_INVOKABLE bool saveToFile(const QString& fileName);
    QByteArray  saveToByteArray();
    Q_INVOKABLE QString saveToString();
    Q_INVOKABLE QString lastError();
    void setCurrentReportsDir(const QString& dirName);
    Q_INVOKABLE void setReportName(const QString& name);
    Q_INVOKABLE QString reportName();
    void setPreviewWindowTitle(const QString& title);
    void setPreviewWindowIcon(const QIcon& icon);
    void setPreviewPageBackgroundColor(QColor color);
    void setResultEditable(bool value);
    bool resultIsEditable();
    void setSaveToFileVisible(bool value);
    bool saveToFileIsVisible();
    void setPrintToPdfVisible(bool value);
    bool printToPdfIsVisible();
    void setPrintVisible(bool value);
    bool printIsVisible();
    bool isBusy();
    void setPassPhrase(QString& passPhrase);
    QList<QLocale::Language> availableLanguages();
    bool setReportLanguage(QLocale::Language language);
    Qt::LayoutDirection previewLayoutDirection();
    void setPreviewLayoutDirection(const Qt::LayoutDirection& previewLayoutDirection);
    QList<QLocale::Language> designerLanguages();
    QLocale::Language currentDesignerLanguage();
    ScaleType previewScaleType();
    int  previewScalePercent();
    void setPreviewScaleType(const ScaleType &previewScaleType, int percent = 0);
    void addWatermark(const WatermarkSetting& watermarkSetting);
    void clearWatermarks();
    IPreparedPages* preparedPages();
    bool showPreparedPages(PreviewHints hints = PreviewBarsUserSetting);
    bool prepareReportPages();
    bool printPreparedPages();
    bool showPreviewModal() const;
    void setShowPreviewModal(bool value);
    bool showDesignerModal() const;
    void setShowDesignerModal(bool showDesignerModal);

signals:
    void cleared();
    void renderStarted();
    void renderFinished();
    void renderPageFinished(int renderedPageCount);

    void printingStarted(int pageCount);
    void printingFinished();
    void pagePrintingFinished(int index);

    void onSave(bool& saved);
    void onSaveAs(bool& saved);
    void onLoad(bool& loaded);
    void onSavePreview(bool& saved, LimeReport::IPreparedPages* pages);
    void saveFinished();
    void loadFinished();
    void printedToPDF(QString fileName);

    void getAvailableDesignerLanguages(QList<QLocale::Language>* languages);
    void currentDefaultDesignerLanguageChanged(QLocale::Language);
    QLocale::Language getCurrentDefaultDesignerLanguage();

    void  externalPaint(const QString& objectName, QPainter* painter, const StyleOptionGraphicsItem*);

public slots:
    void cancelRender();
    void cancelPrinting();
protected:
    ReportEnginePrivate * const d_ptr;
    ReportEngine(ReportEnginePrivate &dd, QObject * parent=0);
private:
    Q_DECLARE_PRIVATE(ReportEngine)
    static QSettings* m_settings;
    bool m_showDesignerModal;
    bool m_showPreviewModal;
};

} // namespace LimeReport

#endif // LRREPORTDESIGNINTF_H
