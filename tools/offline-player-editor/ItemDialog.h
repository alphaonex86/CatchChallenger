#ifndef ITEMDIALOG_H
#define ITEMDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class ItemDialog;
}

class ItemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ItemDialog(QWidget *parent = 0);
    ~ItemDialog();
    bool haveItemSelected();
    quint32 itemId();
    quint64 itemQuantity();
private:
    Ui::ItemDialog *ui;
    QHash<QListWidgetItem *,quint32> items_graphical;
    bool ok;
private slots:
    void displayItems();
    void on_add_clicked();
    void on_search_button_clicked();
};

#endif // ITEMDIALOG_H
