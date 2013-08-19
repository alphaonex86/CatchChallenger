#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QInputDialog>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings("CatchChallenger","item-editor",parent)
{
    ui->setupUi(this);
    ui->lineEditItemFile->setText(settings.value("file").toString());
    if(ui->lineEditItemFile->text().isEmpty())
        ui->browseItemFile->setFocus();
    else
        ui->openItemFile->setFocus();
    ui->stackedWidget->setCurrentWidget(ui->page_welcome);
    loadingTheInformations=false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_browseItemFile_clicked()
{
    QString file=QFileDialog::getOpenFileName(this,"Open the Item file",QString(),tr("Item file")+" (*.xml)");
    if(!file.isEmpty())
        ui->lineEditItemFile->setText(file);
}

void MainWindow::on_openItemFile_clicked()
{
    if(ui->lineEditItemFile->text().isEmpty())
    {
        QMessageBox::warning(this,tr("Error"),tr("You need select a Item file"));
        return;
    }
    if(!ui->lineEditItemFile->text().contains(QRegExp("\\.xml$")))
    {
        QMessageBox::warning(this,tr("Error"),tr("The Item file is a xml file"));
        return;
    }
    QFile xmlFile(ui->lineEditItemFile->text());
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    settings.setValue("file",ui->lineEditItemFile->text());
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
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\nThe root balise is not Items").arg(xmlFile.fileName()));
        return;
    }
    bool error=false;
    //load the Items
    QDomElement child = root.firstChildElement("item");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id") || !child.hasAttribute("price"))
        {
            qDebug() << QString("Has not attribute \"id\" or \"price\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
            error=true;
        }
        else if(!child.isElement())
        {
            qDebug() << QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
            error=true;
        }
        else
        {
            quint32 id=child.attribute("id").toUInt(&ok);
            if(ok)
                items[id]=child;
            else
            {
                qDebug() << QString("Attribute \"id\" is not a number: Item.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
                error=true;
            }
        }
        child = child.nextSiblingElement("item");
    }
    if(error)
        QMessageBox::warning(this,tr("Error"),tr("Some error have been found into the file").arg(xmlFile.fileName()));
    ui->stackedWidget->setCurrentWidget(ui->page_list);
    updateItemList();
}

void MainWindow::updateItemList()
{
    ui->itemList->clear();
    QHash<quint32,QDomElement>::const_iterator i = items.constBegin();
    while (i != items.constEnd()) {
        QListWidgetItem *item=new QListWidgetItem(ui->itemList);
        item->setText(tr("Item %1").arg(i.key()));
        item->setData(99,i.key());
        ui->itemList->addItem(item);
        ++i;
    }
}

void MainWindow::on_itemList_itemDoubleClicked(QListWidgetItem *item)
{
    loadingTheInformations=true;
    quint32 selectedItem=item->data(99).toUInt();
    if(!items[selectedItem].hasAttribute("price"))
        items[selectedItem].setAttribute("price",0);
    bool ok;
    ui->price->setValue(items[selectedItem].attribute("price").toUInt(&ok));
    if(!ok)
        ui->price->setValue(0);
    if(items[selectedItem].hasAttribute("image"))
    {
        QPixmap imageLoaded(QFileInfo(ui->lineEditItemFile->text()).absolutePath()+"/"+items[selectedItem].attribute("image"));
        imageLoaded.scaled(96,96);
        ui->image->setPixmap(imageLoaded);
    }
    else
        ui->image->setPixmap(QPixmap());
    //load name
    {
        ui->nameEditLanguageList->clear();
        QDomElement name = items[selectedItem].firstChildElement("name");
        while(!name.isNull())
        {
            if(!name.hasAttribute("lang"))
                ui->nameEditLanguageList->addItem("en");
            else
                ui->nameEditLanguageList->addItem(name.attribute("lang"));
            name = name.nextSiblingElement("name");
        }
    }
    //load description
    {
        ui->descriptionEditLanguageList->clear();
        QDomElement description = items[selectedItem].firstChildElement("description");
        while(!description.isNull())
        {
            if(!description.hasAttribute("lang"))
                ui->descriptionEditLanguageList->addItem("en");
            else
                ui->descriptionEditLanguageList->addItem(description.attribute("lang"));
            description = description.nextSiblingElement("description");
        }
    }
    ui->stackedWidget->setCurrentWidget(ui->page_edit);
    ui->tabWidget->setCurrentWidget(ui->tabGeneral);
    loadingTheInformations=false;
    on_nameEditLanguageList_currentIndexChanged(ui->nameEditLanguageList->currentIndex());
    on_descriptionEditLanguageList_currentIndexChanged(ui->descriptionEditLanguageList->currentIndex());
}

void MainWindow::on_itemListAdd_clicked()
{
    int index=0;
    while(items.contains(index))
        index++;
    QDomElement newXmlElement=domDocument.createElement("item");
    newXmlElement.setAttribute("id",index);
    domDocument.documentElement().appendChild(newXmlElement);
    updateItemList();
}

void MainWindow::on_itemListEdit_clicked()
{
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    on_itemList_itemDoubleClicked(itemsUI.first());
}

void MainWindow::on_itemListDelete_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->itemList->selectedItems();
    if(selectedItems.size()!=1)
    {
        QMessageBox::warning(this,tr("Error"),tr("Select a bot into the bot list"));
        return;
    }
    quint32 id=selectedItems.first()->data(99).toUInt();
    if(!items.contains(id))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable remove the bot, because the returned id is not into the list"));
        return;
    }
    bool ok;
    //load the bots
    QDomElement child = domDocument.documentElement().firstChildElement("bot");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id"))
            qDebug() << QString("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
        else if(!child.isElement())
            qDebug() << QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
        else
        {
            quint8 tempId=child.attribute("id").toUShort(&ok);
            if(ok)
            {
                if(tempId==id)
                {
                    child.parentNode().removeChild(child);
                    break;
                }
            }
            else
                qDebug() << QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
        }
        child = child.nextSiblingElement("bot");
    }
    items.remove(id);
    updateItemList();
}

void MainWindow::on_itemListSave_clicked()
{
    QFile xmlFile(ui->lineEditItemFile->text());
    if(!xmlFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlFile.write(domDocument.toByteArray(4));
    xmlFile.close();
    QMessageBox::information(this,tr("Saved"),tr("The file have been correctly saved"));
}

void MainWindow::on_nameEditLanguageList_currentIndexChanged(int index)
{
    if(loadingTheInformations)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    Q_UNUSED(index);
    loadingTheInformations=true;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    QDomElement name = items[selectedItem].firstChildElement("name");
    while(!name.isNull())
    {
        if((!name.hasAttribute("lang") && ui->nameEditLanguageList->currentText()=="en")
                ||
                (name.hasAttribute("lang") && ui->nameEditLanguageList->currentText()==name.attribute("lang"))
                )
        {
            ui->namePlainTextEdit->setPlainText(name.text());
            loadingTheInformations=false;
            return;
        }
        name = name.nextSiblingElement("name");
    }
    loadingTheInformations=false;
    QMessageBox::warning(this,tr("Warning"),tr("Text not found"));
}

void MainWindow::on_descriptionEditLanguageList_currentIndexChanged(int index)
{
    if(loadingTheInformations)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    Q_UNUSED(index);
    loadingTheInformations=true;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    QDomElement description = items[selectedItem].firstChildElement("description");
    while(!description.isNull())
    {
        if((!description.hasAttribute("lang") && ui->descriptionEditLanguageList->currentText()=="en")
                ||
                (description.hasAttribute("lang") && ui->descriptionEditLanguageList->currentText()==description.attribute("lang"))
                )
        {
            ui->descriptionPlainTextEdit->setPlainText(description.text());
            loadingTheInformations=false;
            return;
        }
        description = description.nextSiblingElement("description");
    }
    loadingTheInformations=false;
    QMessageBox::warning(this,tr("Warning"),tr("Text not found"));
}

void MainWindow::on_stepEditBack_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_list);
}

void MainWindow::on_namePlainTextEdit_textChanged()
{
    if(loadingTheInformations)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    QDomElement name = items[selectedItem].firstChildElement("name");
    while(!name.isNull())
    {
        if((!name.hasAttribute("lang") && ui->nameEditLanguageList->currentText()=="en")
                ||
                (name.hasAttribute("lang") && ui->nameEditLanguageList->currentText()==name.attribute("lang"))
                )
        {
            QDomText newTextElement=name.ownerDocument().createTextNode(ui->namePlainTextEdit->toPlainText());
            QDomNodeList nodeList=name.childNodes();
            int sub_index=0;
            while(sub_index<nodeList.size())
            {
                name.removeChild(nodeList.at(sub_index));
                sub_index++;
            }
            name.appendChild(newTextElement);
            return;
        }
        name = name.nextSiblingElement("name");
    }
    QMessageBox::warning(this,tr("Warning"),tr("Text not found"));
}

void MainWindow::on_descriptionPlainTextEdit_textChanged()
{
    if(loadingTheInformations)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    QDomElement description = items[selectedItem].firstChildElement("description");
    while(!description.isNull())
    {
        if((!description.hasAttribute("lang") && ui->descriptionEditLanguageList->currentText()=="en")
                ||
                (description.hasAttribute("lang") && ui->descriptionEditLanguageList->currentText()==description.attribute("lang"))
                )
        {
            QDomText newTextElement=description.ownerDocument().createTextNode(ui->descriptionPlainTextEdit->toPlainText());
            QDomNodeList nodeList=description.childNodes();
            int sub_index=0;
            while(sub_index<nodeList.size())
            {
                description.removeChild(nodeList.at(sub_index));
                sub_index++;
            }
            description.appendChild(newTextElement);
            return;
        }
        description = description.nextSiblingElement("description");
    }
    QMessageBox::warning(this,tr("Warning"),tr("Text not found"));
}

void MainWindow::on_price_editingFinished()
{
    if(loadingTheInformations)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    items[selectedItem].setAttribute("price",ui->price->value());
}

void MainWindow::on_imageBrowse_clicked()
{
    QString image=QFileDialog::getOpenFileName(this,"Item image",QFileInfo(ui->lineEditItemFile->text()).absolutePath(),tr("Images (*.png)"));
    if(image.isEmpty())
        return;
    image=QFileInfo(image).absoluteFilePath();
    QPixmap imageLoaded(image);
    if(imageLoaded.isNull())
    {
        QMessageBox::warning(this,tr("Error"),tr("The image can't be loaded, wrong format?"));
        return;
    }
    imageLoaded.scaled(96,96);
    image.remove(QFileInfo(ui->lineEditItemFile->text()).absolutePath());
    image.replace("\\","/");
    image.remove(QRegularExpression("^/"));
    ui->image->setPixmap(imageLoaded);
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    items[selectedItem].setAttribute("image",image);
}
