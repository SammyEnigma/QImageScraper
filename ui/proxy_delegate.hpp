#ifndef PROXY_DELEGATE_HPP
#define PROXY_DELEGATE_HPP

#include <QItemDelegate>

class proxy_delegate : public QItemDelegate
{
public:
    explicit proxy_delegate(QObject *parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    //void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

#endif // PROXY_DELEGATE_HPP
