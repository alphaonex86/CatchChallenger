#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QString>
#include <QDomDocument>
#include <tinyxml2::XMLElement>
#include <QDebug>
#include <QInputDialog>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings("CatchChallenger","map-meta-editor",parent)
{
    ui->setupUi(this);
    ui->lineEditMapMetaFile->setText(settings.value("file").toString());
    if(ui->lineEditMapMetaFile->text().isEmpty())
        ui->browseMapMetaFile->setFocus();
    else
        ui->openMapMetaFile->setFocus();
    ui->stackedWidget->setCurrentWidget(ui->page_welcome);
    loadingTheInformations=false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_browseMapMetaFile_clicked()
{
    QString file=QFileDialog::getOpenFileName(this,"Open the MapMeta file",QString(),tr("Map meta file")+" (*.xml)");
    if(!file.isEmpty())
        ui->lineEditMapMetaFile->setText(file);
}

void MainWindow::on_openMapMetaFile_clicked()
{
    if(ui->lineEditMapMetaFile->text().isEmpty())
    {
        QMessageBox::warning(this,tr("Error"),tr("You need select a MapMeta file"));
        return;
    }
    if(!ui->lineEditMapMetaFile->text().contains(QRegExp("\\.xml$")))
    {
        QMessageBox::warning(this,tr("Error"),tr("The MapMeta file is a xml file"));
        return;
    }
    QFile xmlFile(ui->lineEditMapMetaFile->text());
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    settings.setValue("file",ui->lineEditMapMetaFile->text());
    QByteArray xmlContent=xmlFile.readAll();
    xmlFile.close();
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\nParse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    bool ok;
    tinyxml2::XMLElement root = domDocument.RootElement();
    if(root.tagName()!="map")
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\nThe root balise is not maps").arg(xmlFile.fileName()));
        return;
    }
    bool error=false;
    //load the attribute
    if(root.hasAttribute("type"))
        ui->type->setText(root.attribute("type"));
    if(root.hasAttribute("backgroundsound"))
        ui->backgroundsound->setText(root.attribute("backgroundsound"));
    if(root.hasAttribute("zone"))
        ui->zone->setText(root.attribute("zone"));
    //load the name
    {
        tinyxml2::XMLElement child = root.FirstChildElement("name");
        while(!child.isNull())
        {
            QString lang="en";
            if(child.hasAttribute("lang"))
                lang=child.attribute("lang");
            if(lang=="en")
                ui->nameWarning->setVisible(false);
            if(child.text().isEmpty())
            {
                child.parentNode().removeChild(child);
                child = child.NextSiblingElement("name");
                continue;
            }
            QListWidgetItem * item=new QListWidgetItem();
            item->setData(99,lang);
            item->setData(98,child.text());
            item->setText(QStringLiteral("%1: %2").arg(lang).arg(child.text()));
            ui->nameList->addItem(item);
            child = child.NextSiblingElement("name");
        }
    }
    //load the grass
    {
        tinyxml2::XMLElement grass = root.FirstChildElement("grass");
        if(!grass.isNull())
        {
            grassTotalLuck=0;
            error=false;
            tinyxml2::XMLElement monster = grass.FirstChildElement("monster");
            while(!monster.isNull())
            {
                if(!monster.hasAttribute("id"))
                {
                    error=true;
                    monster = monster.NextSiblingElement("monster");
                    monster.parentNode().removeChild(monster);
                    continue;
                }
                quint32 id=monster.attribute("id").toUInt(&ok);
                if(!ok)
                {
                    error=true;
                    monster = monster.NextSiblingElement("monster");
                    monster.parentNode().removeChild(monster);
                    continue;
                }
                quint8 minLevel,maxLevel,luck;
                if(monster.hasAttribute("level"))
                {
                    quint32 level=monster.attribute("level").toUShort(&ok);
                    if(!ok)
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                    minLevel=level;
                    maxLevel=level;
                }
                else
                {
                    if(monster.hasAttribute("minLevel") && monster.hasAttribute("maxLevel"))
                    {
                        minLevel=monster.attribute("minLevel").toUShort(&ok);
                        if(!ok)
                        {
                            error=true;
                            monster = monster.NextSiblingElement("monster");
                            monster.parentNode().removeChild(monster);
                            continue;
                        }
                        maxLevel=monster.attribute("maxLevel").toUShort(&ok);
                        if(!ok)
                        {
                            error=true;
                            monster = monster.NextSiblingElement("monster");
                            monster.parentNode().removeChild(monster);
                            continue;
                        }
                    }
                    else
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                }
                if(minLevel==maxLevel)
                {
                    monster.removeAttribute("minLevel");
                    monster.removeAttribute("maxLevel");
                    monster.setAttribute("level",minLevel);
                }
                if(!monster.hasAttribute("luck"))
                    luck=100;
                else
                {
                    luck=monster.attribute("luck").toUShort(&ok);
                    if(!ok)
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                }
                if(luck==100)
                    monster.removeAttribute("luck");
                grassTotalLuck+=luck;
                QListWidgetItem * item=new QListWidgetItem();
                item->setData(99,id);
                item->setData(98,minLevel);
                item->setData(97,maxLevel);
                item->setData(96,luck);
                if(minLevel==maxLevel)
                    item->setText(QStringLiteral("Monster %1, level: %2, luck: %3").arg(id).arg(minLevel).arg(luck));
                else
                    item->setText(QStringLiteral("Monster %1, minLevel: %2, maxLevel: %3, luck: %4").arg(id).arg(minLevel).arg(maxLevel).arg(luck));
                ui->grassList->addItem(item);
                monster = monster.NextSiblingElement("monster");
            }
            QStringList errorList;
            if(error)
                errorList << tr("Monster dropped due to error");
            if(grassTotalLuck!=100 && ui->grassList->count()>0)
                errorList << tr("Total luck different than 100");
            if(ui->grassList->count()<=0)
                grass.parentNode().removeChild(grass);
            if(!errorList.isEmpty())
            {
                ui->grassWarning->setText(tr("Warning: %1").arg(errorList.join(", ")));
                ui->grassWarning->setVisible(true);
            }
            else
                ui->grassWarning->setVisible(false);
        }
        else
            ui->grassWarning->setVisible(false);
    }
    //load the wather
    {
        tinyxml2::XMLElement wather = root.FirstChildElement("wather");
        if(!wather.isNull())
        {
            watherTotalLuck=0;
            error=false;
            tinyxml2::XMLElement monster = wather.FirstChildElement("monster");
            while(!monster.isNull())
            {
                if(!monster.hasAttribute("id"))
                {
                    error=true;
                    monster = monster.NextSiblingElement("monster");
                    monster.parentNode().removeChild(monster);
                    continue;
                }
                quint32 id=monster.attribute("id").toUInt(&ok);
                if(!ok)
                {
                    error=true;
                    monster = monster.NextSiblingElement("monster");
                    monster.parentNode().removeChild(monster);
                    continue;
                }
                quint8 minLevel,maxLevel,luck;
                if(monster.hasAttribute("level"))
                {
                    quint32 level=monster.attribute("level").toUShort(&ok);
                    if(!ok)
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                    minLevel=level;
                    maxLevel=level;
                }
                else
                {
                    if(monster.hasAttribute("minLevel") && monster.hasAttribute("maxLevel"))
                    {
                        minLevel=monster.attribute("minLevel").toUShort(&ok);
                        if(!ok)
                        {
                            error=true;
                            monster = monster.NextSiblingElement("monster");
                            monster.parentNode().removeChild(monster);
                            continue;
                        }
                        maxLevel=monster.attribute("maxLevel").toUShort(&ok);
                        if(!ok)
                        {
                            error=true;
                            monster = monster.NextSiblingElement("monster");
                            monster.parentNode().removeChild(monster);
                            continue;
                        }
                    }
                    else
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                }
                if(minLevel==maxLevel)
                {
                    monster.removeAttribute("minLevel");
                    monster.removeAttribute("maxLevel");
                    monster.setAttribute("level",minLevel);
                }
                if(!monster.hasAttribute("luck"))
                    luck=100;
                else
                {
                    luck=monster.attribute("luck").toUShort(&ok);
                    if(!ok)
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                }
                if(luck==100)
                    monster.removeAttribute("luck");
                watherTotalLuck+=luck;
                QListWidgetItem * item=new QListWidgetItem();
                item->setData(99,id);
                item->setData(98,minLevel);
                item->setData(97,maxLevel);
                item->setData(96,luck);
                if(minLevel==maxLevel)
                    item->setText(QStringLiteral("Monster %1, level: %2, luck: %3").arg(id).arg(minLevel).arg(luck));
                else
                    item->setText(QStringLiteral("Monster %1, minLevel: %2, maxLevel: %3, luck: %4").arg(id).arg(minLevel).arg(maxLevel).arg(luck));
                ui->watherList->addItem(item);
                monster = monster.NextSiblingElement("monster");
            }
            QStringList errorList;
            if(error)
                errorList << tr("Monster dropped due to error");
            if(watherTotalLuck!=100 && ui->watherList->count()>0)
                errorList << tr("Total luck different than 100");
            if(ui->watherList->count()<=0)
                wather.parentNode().removeChild(wather);
            if(!errorList.isEmpty())
            {
                ui->watherWarning->setText(tr("Warning: %1").arg(errorList.join(", ")));
                ui->watherWarning->setVisible(true);
            }
            else
                ui->watherWarning->setVisible(false);
        }
        else
            ui->watherWarning->setVisible(false);
    }
    //load the cave
    {
        tinyxml2::XMLElement cave = root.FirstChildElement("cave");
        if(!cave.isNull())
        {
            caveTotalLuck=0;
            error=false;
            tinyxml2::XMLElement monster = cave.FirstChildElement("monster");
            while(!monster.isNull())
            {
                if(!monster.hasAttribute("id"))
                {
                    error=true;
                    monster = monster.NextSiblingElement("monster");
                    monster.parentNode().removeChild(monster);
                    continue;
                }
                quint32 id=monster.attribute("id").toUInt(&ok);
                if(!ok)
                {
                    error=true;
                    monster = monster.NextSiblingElement("monster");
                    monster.parentNode().removeChild(monster);
                    continue;
                }
                quint8 minLevel,maxLevel,luck;
                if(monster.hasAttribute("level"))
                {
                    quint32 level=monster.attribute("level").toUShort(&ok);
                    if(!ok)
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                    minLevel=level;
                    maxLevel=level;
                }
                else
                {
                    if(monster.hasAttribute("minLevel") && monster.hasAttribute("maxLevel"))
                    {
                        minLevel=monster.attribute("minLevel").toUShort(&ok);
                        if(!ok)
                        {
                            error=true;
                            monster = monster.NextSiblingElement("monster");
                            monster.parentNode().removeChild(monster);
                            continue;
                        }
                        maxLevel=monster.attribute("maxLevel").toUShort(&ok);
                        if(!ok)
                        {
                            error=true;
                            monster = monster.NextSiblingElement("monster");
                            monster.parentNode().removeChild(monster);
                            continue;
                        }
                    }
                    else
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                }
                if(minLevel==maxLevel)
                {
                    monster.removeAttribute("minLevel");
                    monster.removeAttribute("maxLevel");
                    monster.setAttribute("level",minLevel);
                }
                if(!monster.hasAttribute("luck"))
                    luck=100;
                else
                {
                    luck=monster.attribute("luck").toUShort(&ok);
                    if(!ok)
                    {
                        error=true;
                        monster = monster.NextSiblingElement("monster");
                        monster.parentNode().removeChild(monster);
                        continue;
                    }
                }
                if(luck==100)
                    monster.removeAttribute("luck");
                caveTotalLuck+=luck;
                QListWidgetItem * item=new QListWidgetItem();
                item->setData(99,id);
                item->setData(98,minLevel);
                item->setData(97,maxLevel);
                item->setData(96,luck);
                if(minLevel==maxLevel)
                    item->setText(QStringLiteral("Monster %1, level: %2, luck: %3").arg(id).arg(minLevel).arg(luck));
                else
                    item->setText(QStringLiteral("Monster %1, minLevel: %2, maxLevel: %3, luck: %4").arg(id).arg(minLevel).arg(maxLevel).arg(luck));
                ui->caveList->addItem(item);
                monster = monster.NextSiblingElement("monster");
            }
            QStringList errorList;
            if(error)
                errorList << tr("Monster dropped due to error");
            if(caveTotalLuck!=100 && ui->caveList->count()>0)
                errorList << tr("Total luck different than 100");
            if(ui->caveList->count()<=0)
                cave.parentNode().removeChild(cave);
            if(!errorList.isEmpty())
            {
                ui->caveWarning->setText(tr("Warning: %1").arg(errorList.join(", ")));
                ui->caveWarning->setVisible(true);
            }
            else
                ui->caveWarning->setVisible(false);
        }
        else
            ui->caveWarning->setVisible(false);
    }

    ui->stackedWidget->setCurrentWidget(ui->page_edit);
    ui->tabWidget->setCurrentWidget(ui->tabGeneral);
}

void MainWindow::on_nameAdd_clicked()
{
    QString lang=QInputDialog::getText(this,tr("Language"),tr("Give the language code"));
    if(lang.isEmpty())
        return;
    QString name=QInputDialog::getText(this,tr("Name"),tr("Give the name for this language"));
    if(name.isEmpty())
        return;
    QListWidgetItem * item=new QListWidgetItem();
    item->setData(99,lang);
    item->setData(98,name);
    item->setText(QStringLiteral("%1: %2").arg(lang).arg(name));
    ui->nameList->addItem(item);
    tinyxml2::XMLElement newXmlElement=domDocument.createElement("name");
    newXmlElement.setAttribute("lang",lang);
    QDomText newTextElement=domDocument.createTextNode(name);
    newXmlElement.appendChild(newTextElement);
    domDocument.RootElement().appendChild(newXmlElement);
}

void MainWindow::on_nameRemove_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->nameList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    QString selectedLang=selectedItems.first()->data(99).toString();
    tinyxml2::XMLElement child = domDocument.RootElement().FirstChildElement("name");
    while(!child.isNull())
    {
        QString lang="en";
        if(child.hasAttribute("lang"))
            lang=child.attribute("lang");
        if(lang=="en")
            ui->nameWarning->setVisible(true);
        if(selectedLang==lang)
        {
            child.parentNode().removeChild(child);
            break;
        }
        child = child.NextSiblingElement("name");
    }
    delete selectedItems.first();
}

void MainWindow::on_type_editingFinished()
{
    if(ui->type->text().isEmpty())
        domDocument.RootElement().removeAttribute("type");
    else
        domDocument.RootElement().setAttribute("type",ui->type->text());
}

void MainWindow::on_backgroundsound_editingFinished()
{
    if(ui->backgroundsound->text().isEmpty())
        domDocument.RootElement().removeAttribute("backgroundsound");
    else
        domDocument.RootElement().setAttribute("backgroundsound",ui->backgroundsound->text());
}

void MainWindow::on_zone_editingFinished()
{
    if(ui->zone->text().isEmpty())
        domDocument.RootElement().removeAttribute("zone");
    else
        domDocument.RootElement().setAttribute("zone",ui->zone->text());
}

void MainWindow::on_save_clicked()
{
    QFile xmlFile(ui->lineEditMapMetaFile->text());
    if(!xmlFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlFile.write(domDocument.toByteArray(4));
    xmlFile.close();
    QMessageBox::information(this,tr("Saved"),tr("The file have been correctly saved"));
}

void MainWindow::on_grassAdd_clicked()
{
    bool ok;
    quint32 id=QInputDialog::getInt(this,tr("Monster"),tr("Give the monster id"),1,1,2000000000,1,&ok);
    if(!ok)
        return;
    quint8 minLevel=QInputDialog::getInt(this,tr("Level"),tr("Minimum level"),1,1,100,1,&ok);
    if(!ok)
        return;
    quint8 maxLevel=QInputDialog::getInt(this,tr("Level"),tr("Maximum level"),1,1,100,1,&ok);
    if(!ok)
        return;
    quint8 luck=QInputDialog::getInt(this,tr("Luck"),tr("Give the luck to find it"),1,1,100,1,&ok);
    if(!ok)
        return;
    grassTotalLuck+=luck;
    if(grassTotalLuck!=100)
    {
        ui->grassWarning->setText(tr("Warning: Total luck different than 100"));
        ui->grassWarning->setVisible(true);
    }
    else
        ui->grassWarning->setVisible(false);
    tinyxml2::XMLElement grass = domDocument.RootElement().FirstChildElement("grass");
    if(grass.isNull())
    {
        tinyxml2::XMLElement grass=domDocument.createElement("grass");
        domDocument.RootElement().appendChild(grass);
    }
    tinyxml2::XMLElement monster=domDocument.createElement("monster");
    QListWidgetItem * item=new QListWidgetItem();
    item->setData(99,id);
    item->setData(98,minLevel);
    item->setData(97,maxLevel);
    item->setData(96,luck);
    monster.setAttribute("id",id);
    if(minLevel==maxLevel)
    {
        monster.setAttribute("level",minLevel);
        item->setText(QStringLiteral("Monster %1, level: %2, luck: %3").arg(id).arg(minLevel).arg(luck));
    }
    else
    {
        monster.setAttribute("minLevel",minLevel);
        monster.setAttribute("maxLevel",maxLevel);
        item->setText(QStringLiteral("Monster %1, minLevel: %2, maxLevel: %3, luck: %4").arg(id).arg(minLevel).arg(maxLevel).arg(luck));
    }
    monster.setAttribute("luck",luck);
    ui->grassList->addItem(item);
    grass.appendChild(monster);
}

void MainWindow::on_grassRemove_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->grassList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    tinyxml2::XMLElement grass = domDocument.RootElement().FirstChildElement("grass");
    if(!grass.isNull())
    {
        tinyxml2::XMLElement monster = grass.FirstChildElement("monster");
        while(!monster.isNull())
        {
            if(monster.hasAttribute("id") && monster.attribute("id")==selectedItems.first()->data(99))
            {
                monster.parentNode().removeChild(monster);
                grassTotalLuck-=selectedItems.first()->data(96).toUInt();
                if(grassTotalLuck!=100)
                {
                    ui->grassWarning->setText(tr("Warning: Total luck different than 100"));
                    ui->grassWarning->setVisible(true);
                }
                else
                    ui->grassWarning->setVisible(false);
                break;
            }
            monster = monster.NextSiblingElement("monster");
        }
        delete selectedItems.first();
        if(ui->grassList->count()<=0)
            grass.parentNode().removeChild(grass);
    }
    else
        delete selectedItems.first();
}

void MainWindow::on_watherAdd_clicked()
{
    bool ok;
    quint32 id=QInputDialog::getInt(this,tr("Monster"),tr("Give the monster id"),1,1,2000000000,1,&ok);
    if(!ok)
        return;
    quint8 minLevel=QInputDialog::getInt(this,tr("Level"),tr("Minimum level"),1,1,100,1,&ok);
    if(!ok)
        return;
    quint8 maxLevel=QInputDialog::getInt(this,tr("Level"),tr("Maximum level"),1,1,100,1,&ok);
    if(!ok)
        return;
    quint8 luck=QInputDialog::getInt(this,tr("Luck"),tr("Give the luck to find it"),1,1,100,1,&ok);
    if(!ok)
        return;
    watherTotalLuck+=luck;
    if(watherTotalLuck!=100)
    {
        ui->watherWarning->setText(tr("Warning: Total luck different than 100"));
        ui->watherWarning->setVisible(true);
    }
    else
        ui->watherWarning->setVisible(false);
    tinyxml2::XMLElement wather = domDocument.RootElement().FirstChildElement("wather");
    if(wather.isNull())
    {
        tinyxml2::XMLElement wather=domDocument.createElement("wather");
        domDocument.RootElement().appendChild(wather);
    }
    tinyxml2::XMLElement monster=domDocument.createElement("monster");
    QListWidgetItem * item=new QListWidgetItem();
    item->setData(99,id);
    item->setData(98,minLevel);
    item->setData(97,maxLevel);
    item->setData(96,luck);
    monster.setAttribute("id",id);
    if(minLevel==maxLevel)
    {
        monster.setAttribute("level",minLevel);
        item->setText(QStringLiteral("Monster %1, level: %2, luck: %3").arg(id).arg(minLevel).arg(luck));
    }
    else
    {
        monster.setAttribute("minLevel",minLevel);
        monster.setAttribute("maxLevel",maxLevel);
        item->setText(QStringLiteral("Monster %1, minLevel: %2, maxLevel: %3, luck: %4").arg(id).arg(minLevel).arg(maxLevel).arg(luck));
    }
    monster.setAttribute("luck",luck);
    ui->watherList->addItem(item);
    wather.appendChild(monster);
}

void MainWindow::on_watherRemove_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->watherList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    tinyxml2::XMLElement wather = domDocument.RootElement().FirstChildElement("wather");
    if(!wather.isNull())
    {
        tinyxml2::XMLElement monster = wather.FirstChildElement("monster");
        while(!monster.isNull())
        {
            if(monster.hasAttribute("id") && monster.attribute("id")==selectedItems.first()->data(99))
            {
                monster.parentNode().removeChild(monster);
                watherTotalLuck-=selectedItems.first()->data(96).toUInt();
                if(watherTotalLuck!=100)
                {
                    ui->watherWarning->setText(tr("Warning: Total luck different than 100"));
                    ui->watherWarning->setVisible(true);
                }
                else
                    ui->watherWarning->setVisible(false);
                break;
            }
            monster = monster.NextSiblingElement("monster");
        }
        delete selectedItems.first();
        if(ui->watherList->count()<=0)
            wather.parentNode().removeChild(wather);
    }
    else
        delete selectedItems.first();
}

void MainWindow::on_caveAdd_clicked()
{
    bool ok;
    quint32 id=QInputDialog::getInt(this,tr("Monster"),tr("Give the monster id"),1,1,2000000000,1,&ok);
    if(!ok)
        return;
    quint8 minLevel=QInputDialog::getInt(this,tr("Level"),tr("Minimum level"),1,1,100,1,&ok);
    if(!ok)
        return;
    quint8 maxLevel=QInputDialog::getInt(this,tr("Level"),tr("Maximum level"),1,1,100,1,&ok);
    if(!ok)
        return;
    quint8 luck=QInputDialog::getInt(this,tr("Luck"),tr("Give the luck to find it"),1,1,100,1,&ok);
    if(!ok)
        return;
    caveTotalLuck+=luck;
    if(caveTotalLuck!=100)
    {
        ui->caveWarning->setText(tr("Warning: Total luck different than 100"));
        ui->caveWarning->setVisible(true);
    }
    else
        ui->caveWarning->setVisible(false);
    tinyxml2::XMLElement cave = domDocument.RootElement().FirstChildElement("cave");
    if(cave.isNull())
    {
        tinyxml2::XMLElement cave=domDocument.createElement("cave");
        domDocument.RootElement().appendChild(cave);
    }
    tinyxml2::XMLElement monster=domDocument.createElement("monster");
    QListWidgetItem * item=new QListWidgetItem();
    item->setData(99,id);
    item->setData(98,minLevel);
    item->setData(97,maxLevel);
    item->setData(96,luck);
    monster.setAttribute("id",id);
    if(minLevel==maxLevel)
    {
        monster.setAttribute("level",minLevel);
        item->setText(QStringLiteral("Monster %1, level: %2, luck: %3").arg(id).arg(minLevel).arg(luck));
    }
    else
    {
        monster.setAttribute("minLevel",minLevel);
        monster.setAttribute("maxLevel",maxLevel);
        item->setText(QStringLiteral("Monster %1, minLevel: %2, maxLevel: %3, luck: %4").arg(id).arg(minLevel).arg(maxLevel).arg(luck));
    }
    monster.setAttribute("luck",luck);
    ui->caveList->addItem(item);
    cave.appendChild(monster);
}

void MainWindow::on_caveRemove_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->caveList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    tinyxml2::XMLElement cave = domDocument.RootElement().FirstChildElement("cave");
    if(!cave.isNull())
    {
        tinyxml2::XMLElement monster = cave.FirstChildElement("monster");
        while(!monster.isNull())
        {
            if(monster.hasAttribute("id") && monster.attribute("id")==selectedItems.first()->data(99))
            {
                monster.parentNode().removeChild(monster);
                caveTotalLuck-=selectedItems.first()->data(96).toUInt();
                if(caveTotalLuck!=100)
                {
                    ui->caveWarning->setText(tr("Warning: Total luck different than 100"));
                    ui->caveWarning->setVisible(true);
                }
                else
                    ui->caveWarning->setVisible(false);
                break;
            }
            monster = monster.NextSiblingElement("monster");
        }
        delete selectedItems.first();
        if(ui->caveList->count()<=0)
            cave.parentNode().removeChild(cave);
    }
    else
        delete selectedItems.first();
}
