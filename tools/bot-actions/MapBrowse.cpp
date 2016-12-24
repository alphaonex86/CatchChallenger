#include "MapBrowse.h"
#include "ui_MapBrowse.h"

MapBrowse::MapBrowse(std::vector<std::string> mapList, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MapBrowse),
    mapList(mapList)
{
    ui->setupUi(this);
    updateFilter();
}

MapBrowse::~MapBrowse()
{
    delete ui;
}

void MapBrowse::on_lineEdit_textChanged(const QString &arg1)
{
    (void)arg1;
    updateFilter();
}

void MapBrowse::updateFilter()
{
    ui->mapList->clear();
    if(ui->filter->text().isEmpty())
    {
        unsigned int index=0;
        while(index<mapList.size())
        {
            ui->mapList->addItem(QString::fromStdString(mapList.at(index)));
            index++;
        }
    }
    else
    {
        unsigned int index=0;
        while(index<mapList.size())
        {
            const QString &string=QString::fromStdString(mapList.at(index));
            if(string.contains(ui->filter->text()))
                ui->mapList->addItem(string);
            index++;
        }
    }
}

void MapBrowse::on_buttonBox_rejected()
{
    mapSelectedString.clear();
    mapList.clear();
    ui->mapList->clear();
    ui->filter->clear();
    reject();
}

void MapBrowse::on_mapList_activated(const QModelIndex &index)
{
    (void)index;
    on_buttonBox_accepted();
}

void MapBrowse::on_buttonBox_accepted()
{
    const QList<QListWidgetItem*> &items=ui->mapList->selectedItems();
    if(items.size()==1)
        mapSelectedString=items.at(0)->text().toStdString();
    else
        mapSelectedString.clear();
    accept();
}

std::string MapBrowse::mapSelected()//empty is not selected
{
    return mapSelectedString;
}

void MapBrowse::on_filter_textEdited(const QString &arg1)
{
    (void)arg1;
    updateFilter();
}
