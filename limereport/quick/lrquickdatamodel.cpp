#include "lrquickdatamodel.h"

#include "lrbanddesignintf.h"
#include "lrbasedesignintf.h"
#include "lrdatadesignintf.h"
#include "lrdatasourcemanager.h"
#include "lrpagedesignintf.h"

#include <QSqlDatabase>

#include <algorithm>

namespace LimeReport {

QuickTreeModel::Node* QuickTreeModel::Node::add(const QString& label, const QString& name,
                                                const QString& kind)
{
    Node* node = new Node;
    node->label = label;
    node->name = name;
    node->kind = kind;
    node->parent = this;
    node->depth = depth + 1;
    node->key = key + "/" + kind + ":" + name;
    children.append(node);
    return node;
}

QuickTreeModel::QuickTreeModel(QObject* parent):
    QAbstractListModel(parent),
    m_root(nullptr),
    m_defaultExpanded(true)
{
}

QuickTreeModel::~QuickTreeModel() { delete m_root; }

int QuickTreeModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.count();
}

QHash<int, QByteArray> QuickTreeModel::roleNames() const
{
    return { { LabelRole, "label" },
             { NameRole, "name" },
             { KindRole, "kind" },
             { DepthRole, "depth" },
             { HasChildrenRole, "hasChildren" },
             { ExpandedRole, "expanded" },
             { DragTextRole, "dragText" },
             { StatusRole, "status" },
             { SelectedRole, "selected" },
             { ObjectRole, "object" } };
}

QVariant QuickTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.count())
        return QVariant();
    const Node* node = m_rows.at(index.row());
    switch (role) {
    case LabelRole:
        return node->label;
    case NameRole:
        return node->name;
    case KindRole:
        return node->kind;
    case DepthRole:
        return node->depth - 1;
    case HasChildrenRole:
        return !node->children.isEmpty();
    case ExpandedRole:
        return !node->children.isEmpty()
            && (m_expanded.contains(node->key)
                || (m_defaultExpanded && !m_collapsed.contains(node->key)));
    case DragTextRole:
        return node->dragText;
    case StatusRole:
        return node->status;
    case SelectedRole:
        return isSelected(node);
    case ObjectRole:
        return QVariant::fromValue<QObject*>(node->object.data());
    }
    return QVariant();
}

QVariantMap QuickTreeModel::get(int row) const
{
    QVariantMap result;
    if (row < 0 || row >= m_rows.count())
        return result;
    QHash<int, QByteArray> roles = roleNames();
    for (auto it = roles.constBegin(); it != roles.constEnd(); ++it)
        result.insert(QString::fromLatin1(it.value()), data(index(row), it.key()));
    return result;
}

int QuickTreeModel::rowOfObject(QObject* object) const
{
    for (int i = 0; i < m_rows.count(); ++i)
        if (m_rows.at(i)->object == object)
            return i;
    return -1;
}

void QuickTreeModel::flatten(Node* node)
{
    for (Node* child : node->children) {
        m_rows.append(child);
        bool expanded = m_expanded.contains(child->key)
            || (m_defaultExpanded && !m_collapsed.contains(child->key));
        if (expanded)
            flatten(child);
    }
}

void QuickTreeModel::rebuild()
{
    beginResetModel();
    delete m_root;
    m_rows.clear();
    m_root = new Node;
    m_root->depth = 0;
    build(m_root);
    flatten(m_root);
    endResetModel();
}

void QuickTreeModel::toggle(int row)
{
    if (row < 0 || row >= m_rows.count())
        return;
    Node* node = m_rows.at(row);
    if (node->children.isEmpty())
        return;
    bool expanded
        = m_expanded.contains(node->key) || (m_defaultExpanded && !m_collapsed.contains(node->key));
    if (expanded) {
        m_expanded.remove(node->key);
        m_collapsed.insert(node->key);
    } else {
        m_collapsed.remove(node->key);
        m_expanded.insert(node->key);
    }
    beginResetModel();
    m_rows.clear();
    flatten(m_root);
    endResetModel();
}

void QuickTreeModel::expandAll()
{
    m_collapsed.clear();
    setDefaultExpanded(true);
    beginResetModel();
    m_rows.clear();
    if (m_root)
        flatten(m_root);
    endResetModel();
}

void QuickTreeModel::refreshSelection()
{
    if (m_rows.isEmpty())
        return;
    emit dataChanged(index(0), index(m_rows.count() - 1), { SelectedRole });
}

// ---------------------------------------------------------------------------

QuickDataBrowserModel::QuickDataBrowserModel(QObject* parent): QuickTreeModel(parent) { }

void QuickDataBrowserModel::setDataManager(DataSourceManager* dataManager)
{
    if (m_dataManager == dataManager)
        return;
    if (m_dataManager)
        disconnect(m_dataManager, nullptr, this, nullptr);
    m_dataManager = dataManager;
    if (m_dataManager) {
        connect(m_dataManager, &DataSourceManager::datasourcesChanged, this,
                &QuickTreeModel::rebuild);
        connect(m_dataManager, &DataSourceManager::cleared, this, &QuickTreeModel::rebuild);
    }
    rebuild();
}

void QuickDataBrowserModel::build(Node* root)
{
    if (!m_dataManager)
        return;
    DataSourceManager* dm = m_dataManager;

    Node* datasources = root->add(tr("Datasources"), QString(), "category");

    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString& connectionName : dm->connectionNames())
        if (!connections.contains(connectionName, Qt::CaseInsensitive))
            connections.append(connectionName);
    std::sort(connections.begin(), connections.end());

    QMap<QString, Node*> connectionNodes;
    for (const QString& connectionName : connections) {
        QString userName = ConnectionDesc::connectionNameForUser(connectionName);
        Node* node = datasources->add(userName, connectionName, "connection");
        bool internal = dm->connectionNames().contains(
            ConnectionDesc::connectionNameForReport(connectionName), Qt::CaseInsensitive);
        node->status = (!internal || dm->isConnectionConnected(connectionName)) ? "connected"
                                                                                : "disconnected";
        connectionNodes.insert(userName, node);
    }

    QStringList names = dm->dataSourceNames();
    std::sort(names.begin(), names.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    for (const QString& name : names) {
        QString connectionName = ConnectionDesc::connectionNameForUser(dm->connectionName(name));
        Node* parent = connectionNodes.value(connectionName, datasources);
        QString kind = "datasource";
        if (dm->isQuery(name))
            kind = "query";
        else if (dm->isSubQuery(name))
            kind = "subquery";
        else if (dm->isProxy(name))
            kind = "proxy";
        else if (dm->isCSV(name))
            kind = "csv";
        Node* dsNode = parent->add(name, name, kind);
        try {
            IDataSource* ds = dm->dataSource(name);
            if (ds) {
                dsNode->status = ds->isInvalid() ? "error" : "ok";
                QStringList fields;
                for (int i = 0; i < ds->columnCount(); ++i)
                    fields << ds->columnNameByIndex(i);
                fields.sort(Qt::CaseInsensitive);
                for (const QString& field : fields) {
                    Node* f = dsNode->add(field, field, "field");
                    f->dragText = "field:$D{" + name + "." + field + "}";
                }
            } else {
                dsNode->status = "error";
            }
        } catch (ReportError&) {
            dsNode->status = "error";
        }
    }

    Node* variables = root->add(tr("Variables"), QString(), "category");
    Node* reportVariables = variables->add(tr("Report variables"), "report", "category");
    Node* systemVariables = variables->add(tr("System variables"), "system", "category");
    Node* externalVariables = variables->add(tr("External variables"), "external", "category");

    QStringList variableNames = dm->variableNames();
    for (const QString& variableName : variableNames) {
        bool system = dm->variableIsSystem(variableName);
        QString label = system ? variableName
                               : variableName + "  [" + dm->variable(variableName).toString() + "]";
        Node* v = (system ? systemVariables : reportVariables)
                      ->add(label, variableName, system ? "systemVariable" : "variable");
        v->dragText = "variable:$V{" + variableName + "}";
    }
    for (const QString& variableName : dm->userVariableNames()) {
        if (variableNames.contains(variableName))
            continue;
        Node* v = externalVariables->add(variableName + "  ["
                                             + dm->variable(variableName).toString() + "]",
                                         variableName, "externalVariable");
        v->dragText = "variable:$V{" + variableName + "}";
    }
}

// ---------------------------------------------------------------------------

QuickObjectTreeModel::QuickObjectTreeModel(QObject* parent): QuickTreeModel(parent) { }

void QuickObjectTreeModel::setPage(PageDesignIntf* page)
{
    if (m_page == page)
        return;
    if (m_page)
        disconnect(m_page, nullptr, this, nullptr);
    m_page = page;
    if (m_page) {
        connect(m_page, &PageDesignIntf::itemAdded, this, &QuickTreeModel::rebuild);
        connect(m_page, &PageDesignIntf::itemRemoved, this, &QuickTreeModel::rebuild);
        connect(m_page, &PageDesignIntf::bandAdded, this, &QuickTreeModel::rebuild);
        connect(m_page, &PageDesignIntf::bandRemoved, this, &QuickTreeModel::rebuild);
        connect(m_page, &PageDesignIntf::itemPropertyObjectNameChanged, this,
                &QuickTreeModel::rebuild);
        connect(m_page, &PageDesignIntf::pageUpdateFinished, this, &QuickTreeModel::rebuild);
        connect(m_page, &GraphicsScene::selectionChanged, this,
                &QuickObjectTreeModel::updateSelection);
    }
    rebuild();
}

void QuickObjectTreeModel::addItem(Node* parent, BaseDesignIntf* item)
{
    QString type = QString::fromLatin1(item->metaObject()->className());
    int pos = type.lastIndexOf("::");
    if (pos >= 0)
        type = type.mid(pos + 2);
    Node* node = parent->add(item->objectName(), item->objectName(), type);
    node->object = item;
    QList<BaseDesignIntf*> children = item->childBaseItems();
    std::stable_sort(children.begin(), children.end(), [](BaseDesignIntf* a, BaseDesignIntf* b) {
        if (a->isBand() && b->isBand())
            return a->y() < b->y();
        if (a->isBand() != b->isBand())
            return a->isBand();
        return a->objectName().compare(b->objectName(), Qt::CaseInsensitive) < 0;
    });
    for (BaseDesignIntf* child : children)
        addItem(node, child);
}

void QuickObjectTreeModel::build(Node* root)
{
    if (!m_page || !m_page->pageItem())
        return;
    addItem(root, m_page->pageItem());
}

bool QuickObjectTreeModel::isSelected(const Node* node) const
{
    BaseDesignIntf* item = qobject_cast<BaseDesignIntf*>(node->object.data());
    return item && item->isSelected();
}

} // namespace LimeReport
