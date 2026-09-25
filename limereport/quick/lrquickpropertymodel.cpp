#include "lrquickpropertymodel.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QRect>
#include <QMetaEnum>
#include <QFileInfo>
#include <algorithm>

#include "lrbasedesignintf.h"
#include "lrdatasourcemanager.h"

namespace LimeReport {

namespace {

int variantToInt(const QVariant& value)
{
    bool ok = false;
    int result = value.toInt(&ok);
    if (ok) return result;
    const void* data = value.constData();
    if (!data) return 0;
    switch (value.metaType().sizeOf()) {
    case 1: return *static_cast<const qint8*>(data);
    case 2: return *static_cast<const qint16*>(data);
    case 4: return *static_cast<const qint32*>(data);
    case 8: return int(*static_cast<const qint64*>(data));
    default: return 0;
    }
}

QString stripNamespace(const char* className)
{
    QString name = QString::fromLatin1(className);
    int pos = name.lastIndexOf("::");
    return pos >= 0 ? name.mid(pos + 2) : name;
}

const QStringList& multiLineProperties()
{
    static const QStringList names = {
        "content", "script", "initScript", "queryText", "csvText", "designTestValue",
        "expression", "printCondition", "text"
    };
    return names;
}

const QStringList& datasourceProperties()
{
    static const QStringList names = { "datasource", "datasourceName", "dataSource" };
    return names;
}

} // namespace

QuickPropertyModel::QuickPropertyModel(QObject* parent)
    : QAbstractListModel(parent), m_dataManager(nullptr)
{
}

int QuickPropertyModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_properties.count();
}

QHash<int, QByteArray> QuickPropertyModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { DisplayNameRole, "displayName" },
        { ValueRole, "value" },
        { ValueTextRole, "valueText" },
        { EditorRole, "editor" },
        { OptionsRole, "options" },
        { ReadOnlyRole, "readOnly" },
        { SectionRole, "section" }
    };
}

QList<QObject*> QuickPropertyModel::objects() const
{
    QList<QObject*> result;
    for (const QPointer<QObject>& object : m_objects)
        if (object) result.append(object.data());
    return result;
}

QString QuickPropertyModel::objectName() const
{
    QList<QObject*> list = objects();
    if (list.isEmpty()) return QString();
    if (list.count() > 1) return tr("%1 objects").arg(list.count());
    return list.first()->objectName();
}

QString QuickPropertyModel::objectType() const
{
    QList<QObject*> list = objects();
    if (list.isEmpty()) return QString();
    return stripNamespace(list.first()->metaObject()->className());
}

void QuickPropertyModel::setObjects(const QList<QObject*>& objects)
{
    QList<QObject*> current = this->objects();
    if (current == objects) {
        refresh();
        return;
    }
    beginResetModel();
    m_objects.clear();
    for (QObject* object : objects) m_objects.append(QPointer<QObject>(object));
    rebuild();
    endResetModel();
    emit objectsChanged();
}

QString QuickPropertyModel::editorFor(const QMetaProperty& property) const
{
    const QString name = QString::fromLatin1(property.name());
    if (property.isEnumType()) return property.isFlagType() ? QStringLiteral("flags") : QStringLiteral("enum");
    if (datasourceProperties().contains(name)) return QStringLiteral("datasource");
    switch (property.metaType().id()) {
    case QMetaType::QString:
        return multiLineProperties().contains(name) ? QStringLiteral("text") : QStringLiteral("string");
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::LongLong:
        return QStringLiteral("int");
    case QMetaType::Double:
    case QMetaType::Float:
        return QStringLiteral("real");
    case QMetaType::Bool:
        return QStringLiteral("bool");
    case QMetaType::QColor:
        return QStringLiteral("color");
    case QMetaType::QFont:
        return QStringLiteral("font");
    case QMetaType::QRect:
    case QMetaType::QRectF:
        return QStringLiteral("rect");
    case QMetaType::QImage:
        return QStringLiteral("image");
    case QMetaType::QByteArray:
        return name.contains("image", Qt::CaseInsensitive) ? QStringLiteral("image") : QString();
    default:
        return QString();
    }
}

void QuickPropertyModel::rebuild()
{
    m_properties.clear();
    QList<QObject*> list = objects();
    if (list.isEmpty()) return;

    QObject* first = list.first();
    const QMetaObject* meta = first->metaObject();
    for (int i = 0; i < meta->propertyCount(); ++i) {
        QMetaProperty property = meta->property(i);
        if (!property.isDesignable() || !property.isWritable()) continue;
        QString editor = editorFor(property);
        if (editor.isEmpty()) continue;
        const char* name = property.name();
        bool common = true;
        for (QObject* other : list) {
            if (other->metaObject()->indexOfProperty(name) < 0) { common = false; break; }
        }
        if (!common) continue;
        if (list.count() > 1 && qstrcmp(name, "objectName") == 0) continue;

        const QMetaObject* owner = meta;
        int depth = 0;
        while (owner->superClass() && i < owner->superClass()->propertyCount()) {
            owner = owner->superClass();
            ++depth;
        }

        PropertyInfo info;
        info.name = QString::fromLatin1(name);
        info.editor = editor;
        info.section = stripNamespace(owner->className());
        info.sectionOrder = -depth;
        info.meta = property;
        m_properties.append(info);
    }
    std::stable_sort(m_properties.begin(), m_properties.end(), [](const PropertyInfo& a, const PropertyInfo& b) {
        if (a.sectionOrder != b.sectionOrder) return a.sectionOrder < b.sectionOrder;
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
}

void QuickPropertyModel::refresh()
{
    if (m_properties.isEmpty()) return;
    emit dataChanged(index(0), index(m_properties.count() - 1));
}

qreal QuickPropertyModel::unitFactor() const
{
    QList<QObject*> list = objects();
    BaseDesignIntf* item = list.isEmpty() ? nullptr : dynamic_cast<BaseDesignIntf*>(list.first());
    return item ? item->unitFactor() : 1;
}

QString QuickPropertyModel::unitName() const
{
    QList<QObject*> list = objects();
    BaseDesignIntf* item = list.isEmpty() ? nullptr : dynamic_cast<BaseDesignIntf*>(list.first());
    if (!item) return QString();
    return item->unitType() == BaseDesignIntf::Millimeters ? tr("mm") : tr("in");
}

QVariantList QuickPropertyModel::optionsFor(const PropertyInfo& info) const
{
    QVariantList result;
    if (info.editor == "enum" || info.editor == "flags") {
        QMetaEnum enumerator = info.meta.enumerator();
        for (int i = 0; i < enumerator.keyCount(); ++i) {
            int value = enumerator.value(i);
            if (info.editor == "flags" && value == 0) continue;
            QVariantMap item;
            item.insert("text", QString::fromLatin1(enumerator.key(i)));
            item.insert("value", value);
            result.append(item);
        }
    } else if (info.editor == "datasource" && m_dataManager) {
        QStringList names = m_dataManager->dataSourceNames();
        names.sort(Qt::CaseInsensitive);
        result.append(QString());
        for (const QString& name : names) result.append(name);
    }
    return result;
}

QString QuickPropertyModel::valueText(const PropertyInfo& info, const QVariant& value) const
{
    if (info.editor == "enum") {
        const char* key = info.meta.enumerator().valueToKey(variantToInt(value));
        return key ? QString::fromLatin1(key) : QString::number(variantToInt(value));
    }
    if (info.editor == "flags") {
        return QString::fromLatin1(info.meta.enumerator().valueToKeys(variantToInt(value))).replace('|', ", ");
    }
    if (info.editor == "bool") return value.toBool() ? tr("true") : tr("false");
    if (info.editor == "color") return value.value<QColor>().name(QColor::HexArgb);
    if (info.editor == "font") {
        QFont f = value.value<QFont>();
        QStringList parts;
        parts << f.family() << QString::number(f.pointSizeF()) + "pt";
        if (f.bold()) parts << tr("bold");
        if (f.italic()) parts << tr("italic");
        if (f.underline()) parts << tr("underline");
        return parts.join(", ");
    }
    if (info.editor == "rect") {
        QRectF r = value.toRectF();
        qreal factor = unitFactor();
        return QString("%1, %2, %3 x %4 %5")
                .arg(r.x() / factor, 0, 'f', 2).arg(r.y() / factor, 0, 'f', 2)
                .arg(r.width() / factor, 0, 'f', 2).arg(r.height() / factor, 0, 'f', 2)
                .arg(unitName());
    }
    if (info.editor == "image") {
        if (value.metaType().id() == QMetaType::QImage) {
            QImage image = value.value<QImage>();
            return image.isNull() ? tr("(none)") : QString("%1 x %2").arg(image.width()).arg(image.height());
        }
        return value.toByteArray().isEmpty() ? tr("(none)") : tr("(data)");
    }
    if (info.editor == "text") {
        QString text = value.toString();
        text.replace('\n', ' ');
        return text.length() > 60 ? text.left(57) + "..." : text;
    }
    return value.toString();
}

QVariant QuickPropertyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_properties.count()) return QVariant();
    const PropertyInfo& info = m_properties.at(index.row());
    QList<QObject*> list = objects();
    if (list.isEmpty()) return QVariant();
    QVariant value = list.first()->property(info.name.toLatin1());

    switch (role) {
    case NameRole: return info.name;
    case DisplayNameRole: return info.name;
    case ValueRole:
        if (info.editor == "enum" || info.editor == "flags") return variantToInt(value);
        if (info.editor == "rect") {
            QRectF r = value.toRectF();
            qreal f = unitFactor();
            return QRectF(r.x() / f, r.y() / f, r.width() / f, r.height() / f);
        }
        if (info.editor == "font") {
            QFont font = value.value<QFont>();
            QVariantMap map;
            map.insert("family", font.family());
            map.insert("pointSize", font.pointSizeF());
            map.insert("bold", font.bold());
            map.insert("italic", font.italic());
            map.insert("underline", font.underline());
            map.insert("strikeout", font.strikeOut());
            return map;
        }
        if (info.editor == "image") return QVariant();
        return value;
    case ValueTextRole: return valueText(info, value);
    case EditorRole: return info.editor;
    case OptionsRole: return optionsFor(info);
    case ReadOnlyRole: return !info.meta.isWritable();
    case SectionRole: return info.section;
    }
    return QVariant();
}

bool QuickPropertyModel::writeToAll(const QString& name, const QVariant& value)
{
    bool result = false;
    QByteArray propertyName = name.toLatin1();
    for (QObject* object : objects()) {
        if (object->setProperty(propertyName, value)) result = true;
    }
    refresh();
    emit valueEdited(name);
    return result;
}

bool QuickPropertyModel::setValue(int row, const QVariant& value)
{
    if (row < 0 || row >= m_properties.count()) return false;
    const PropertyInfo& info = m_properties.at(row);
    QVariant v = value;
    if (info.editor == "color") v = QVariant::fromValue(QColor(value.toString().isEmpty() ? value.value<QColor>() : QColor(value.toString())));
    else if (info.editor == "int") v = value.toInt();
    else if (info.editor == "real") v = value.toDouble();
    else if (info.editor == "bool") v = value.toBool();
    else if (info.editor == "enum" || info.editor == "flags") v = value.toInt();
    else if (info.editor == "string" || info.editor == "text" || info.editor == "datasource") v = value.toString();
    return writeToAll(info.name, v);
}

bool QuickPropertyModel::setFlag(int row, int flag, bool on)
{
    if (row < 0 || row >= m_properties.count()) return false;
    const PropertyInfo& info = m_properties.at(row);
    QList<QObject*> list = objects();
    if (list.isEmpty()) return false;
    int current = variantToInt(list.first()->property(info.name.toLatin1()));
    int value = on ? (current | flag) : (current & ~flag);
    return writeToAll(info.name, value);
}

bool QuickPropertyModel::setFontValue(int row, const QString& family, qreal pointSize, bool bold, bool italic,
                                      bool underline, bool strikeOut)
{
    if (row < 0 || row >= m_properties.count()) return false;
    const PropertyInfo& info = m_properties.at(row);
    QList<QObject*> list = objects();
    if (list.isEmpty()) return false;
    QFont font = list.first()->property(info.name.toLatin1()).value<QFont>();
    if (!family.isEmpty()) font.setFamily(family);
    if (pointSize > 0) font.setPointSizeF(pointSize);
    font.setBold(bold);
    font.setItalic(italic);
    font.setUnderline(underline);
    font.setStrikeOut(strikeOut);
    return writeToAll(info.name, font);
}

bool QuickPropertyModel::setRectValue(int row, qreal x, qreal y, qreal width, qreal height)
{
    if (row < 0 || row >= m_properties.count()) return false;
    const PropertyInfo& info = m_properties.at(row);
    qreal f = unitFactor();
    QRectF r(x * f, y * f, width * f, height * f);
    if (info.meta.metaType().id() == QMetaType::QRect) return writeToAll(info.name, r.toRect());
    return writeToAll(info.name, r);
}

bool QuickPropertyModel::loadImage(int row, const QUrl& fileUrl)
{
    if (row < 0 || row >= m_properties.count()) return false;
    const PropertyInfo& info = m_properties.at(row);
    QString fileName = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (info.meta.metaType().id() == QMetaType::QImage) {
        QImage image(fileName);
        if (image.isNull()) return false;
        return writeToAll(info.name, image);
    }
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return false;
    return writeToAll(info.name, file.readAll());
}

bool QuickPropertyModel::clearValue(int row)
{
    if (row < 0 || row >= m_properties.count()) return false;
    const PropertyInfo& info = m_properties.at(row);
    return writeToAll(info.name, QVariant(info.meta.metaType()));
}

int QuickPropertyModel::rowOf(const QString& propertyName) const
{
    for (int i = 0; i < m_properties.count(); ++i)
        if (m_properties.at(i).name == propertyName) return i;
    return -1;
}

} // namespace LimeReport
