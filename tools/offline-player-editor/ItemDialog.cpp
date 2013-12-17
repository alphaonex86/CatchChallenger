#include "ItemDialog.h"
#include "ui_ItemDialog.h"
#include "../../client/base/interface/DatapackClientLoader.h"

ItemDialog::ItemDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ItemDialog)
{
    ui->setupUi(this);
    ok=false;
    displayItems();
}

ItemDialog::~ItemDialog()
{
    delete ui;
}

void ItemDialog::displayItems()
{

    ui->items->clear();
    items_graphical.clear();

    QHashIterator<quint32,DatapackClientLoader::item> i(DatapackClientLoader::items);
    while (i.hasNext()) {
        i.next();
        const DatapackClientLoader::item &item=i.value();
        if(ui->search->text().isEmpty() || item.name.contains(ui->search->text(),Qt::CaseInsensitive) || item.description.contains(ui->search->text(),Qt::CaseInsensitive))
        {
            QListWidgetItem *item=new QListWidgetItem();
            items_graphical[item]=i.key();
            item->setIcon(DatapackClientLoader::items[i.key()].image);
            item->setText(QStringLiteral("%1").arg(DatapackClientLoader::items[i.key()].name));
            item->setToolTip(DatapackClientLoader::items[i.key()].description);
            ui->items->addItem(item);
        }
    }
}

bool ItemDialog::haveItemSelected()
{
    return ok && ui->items->selectedItems().size()==1;
}

quint32 ItemDialog::itemId()
{
    if(!haveItemSelected())
        return 0;
    return items_graphical[ui->items->selectedItems().first()];
}

quint64 ItemDialog::itemQuantity()
{
    if(!haveItemSelected())
        return 0;
    return ui->quantity->value();
}

void ItemDialog::on_add_clicked()
{
    ok=true;
    close();
}

void ItemDialog::on_search_button_clicked()
{
    displayItems();
}
