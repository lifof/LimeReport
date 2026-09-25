#ifndef LRQUICKDATAMODEL_H
#define LRQUICKDATAMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QSet>
#include <QtQml/qqmlregistration.h>

namespace LimeReport {

class DataSourceManager;
class PageDesignIntf;
class BaseDesignIntf;

/*
 * A tree flattened into a list model, so that it can be shown with a plain
 * QML ListView (indentation by "depth", expansion via toggle()).
 */
class QuickTreeModel: public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        LabelRole = Qt::UserRole + 1,
        NameRole,
        KindRole,
        DepthRole,
        HasChildrenRole,
        ExpandedRole,
        DragTextRole,
        StatusRole,
        SelectedRole,
        ObjectRole
    };

    explicit QuickTreeModel(QObject* parent = nullptr);
    ~QuickTreeModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void toggle(int row);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void rebuild();
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE int rowOfObject(QObject* object) const;

protected:
    struct Node {
        QString label;
        QString name;
        QString kind;
        QString dragText;
        QString status;
        QString key;
        QPointer<QObject> object;
        Node* parent = nullptr;
        QList<Node*> children;
        int depth = 0;
        ~Node() { qDeleteAll(children); }
        Node* add(const QString& label, const QString& name, const QString& kind);
    };
    virtual void build(Node* root) = 0;
    virtual bool isSelected(const Node*) const { return false; }
    void refreshSelection();

private:
    void flatten(Node* node);

private:
    Node* m_root;
    QList<Node*> m_rows;
    QSet<QString> m_collapsed;
    QSet<QString> m_expanded;
    bool m_defaultExpanded;

protected:
    void setDefaultExpanded(bool value) { m_defaultExpanded = value; }
};

class QuickDataBrowserModel: public QuickTreeModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ReportDataBrowserModel)
    QML_UNCREATABLE("Provided by ReportDesignerController")
public:
    explicit QuickDataBrowserModel(QObject* parent = nullptr);
    void setDataManager(DataSourceManager* dataManager);

protected:
    void build(Node* root) override;

private:
    QPointer<DataSourceManager> m_dataManager;
};

class QuickObjectTreeModel: public QuickTreeModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ReportObjectTreeModel)
    QML_UNCREATABLE("Provided by ReportDesignerController")
public:
    explicit QuickObjectTreeModel(QObject* parent = nullptr);
    void setPage(PageDesignIntf* page);
    Q_INVOKABLE void updateSelection() { refreshSelection(); }

protected:
    void build(Node* root) override;
    bool isSelected(const Node* node) const override;

private:
    void addItem(Node* parent, BaseDesignIntf* item);
    QPointer<PageDesignIntf> m_page;
};

} // namespace LimeReport

#endif // LRQUICKDATAMODEL_H
