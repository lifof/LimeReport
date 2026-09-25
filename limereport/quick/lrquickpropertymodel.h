#ifndef LRQUICKPROPERTYMODEL_H
#define LRQUICKPROPERTYMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QMetaProperty>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace LimeReport {

class DataSourceManager;

/*
 * List model exposing the designable properties of the selected report
 * object(s) to the QML property inspector. With several objects selected,
 * only common properties are listed and edits are applied to all of them.
 */
class QuickPropertyModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ReportPropertyModel)
    QML_UNCREATABLE("Provided by ReportDesignerController")
    Q_PROPERTY(int objectCount READ objectCount NOTIFY objectsChanged)
    Q_PROPERTY(QString objectName READ objectName NOTIFY objectsChanged)
    Q_PROPERTY(QString objectType READ objectType NOTIFY objectsChanged)
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole,
        ValueRole,
        ValueTextRole,
        EditorRole,
        OptionsRole,
        ReadOnlyRole,
        SectionRole
    };
    Q_ENUM(Roles)

    explicit QuickPropertyModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setObjects(const QList<QObject*>& objects);
    QList<QObject*> objects() const;
    void setDataManager(DataSourceManager* dataManager) { m_dataManager = dataManager; }
    int objectCount() const { return objects().count(); }
    QString objectName() const;
    QString objectType() const;

    Q_INVOKABLE bool setValue(int row, const QVariant& value);
    Q_INVOKABLE bool setFlag(int row, int flag, bool on);
    Q_INVOKABLE bool setFontValue(int row, const QString& family, qreal pointSize, bool bold, bool italic,
                                  bool underline, bool strikeOut);
    Q_INVOKABLE bool setRectValue(int row, qreal x, qreal y, qreal width, qreal height);
    Q_INVOKABLE bool loadImage(int row, const QUrl& fileUrl);
    Q_INVOKABLE bool clearValue(int row);
    Q_INVOKABLE int rowOf(const QString& propertyName) const;
    Q_INVOKABLE void refresh();

signals:
    void objectsChanged();
    void valueEdited(const QString& propertyName);

private:
    struct PropertyInfo {
        QString name;
        QString editor;
        QString section;
        int sectionOrder = 0;
        QMetaProperty meta;
    };
    QString editorFor(const QMetaProperty& property) const;
    QVariantList optionsFor(const PropertyInfo& info) const;
    QString valueText(const PropertyInfo& info, const QVariant& value) const;
    qreal unitFactor() const;
    QString unitName() const;
    bool writeToAll(const QString& name, const QVariant& value);
    void rebuild();

private:
    QList<QPointer<QObject>> m_objects;
    QList<PropertyInfo> m_properties;
    DataSourceManager* m_dataManager;
};

} // namespace LimeReport

#endif // LRQUICKPROPERTYMODEL_H
