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
    if(root.tagName()!="list")
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\nThe root balise is not Items").arg(xmlFile.fileName()));
        return;
    }
    bool error=false;
    //load the Items
    QDomElement child = root.firstChildElement("skill");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id"))
        {
            qDebug() << QStringLiteral("Has not attribute \"id\" or \"price\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
            error=true;
        }
        else if(!child.isElement())
        {
            qDebug() << QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
            error=true;
        }
        else
        {
            quint32 id=child.attribute("id").toUInt(&ok);
            if(ok)
                skills[id]=child;
            else
            {
                qDebug() << QStringLiteral("Attribute \"id\" is not a number: Item.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
                error=true;
            }
        }
        child = child.nextSiblingElement("skill");
    }
    if(error)
        QMessageBox::warning(this,tr("Error"),tr("Some error have been found into the file").arg(xmlFile.fileName()));
    ui->stackedWidget->setCurrentWidget(ui->page_list);
    updateItemList();
}

void MainWindow::updateItemList()
{
    ui->itemList->clear();
    QHash<quint32,QDomElement>::const_iterator i = skills.constBegin();
    while (i != skills.constEnd()) {
        QListWidgetItem *item=new QListWidgetItem(ui->itemList);
        item->setText(tr("Skill %1").arg(i.key()));
        item->setData(99,i.key());
        ui->itemList->addItem(item);
        ++i;
    }
}

void MainWindow::on_itemList_itemDoubleClicked(QListWidgetItem *item)
{
    loadingTheInformations=true;
    updateLevelList();
    quint32 selectedItem=item->data(99).toUInt();
    //load name
    {
        ui->nameEditLanguageList->clear();
        QDomElement name = skills[selectedItem].firstChildElement("name");
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
        QDomElement description = skills[selectedItem].firstChildElement("description");
        while(!description.isNull())
        {
            if(!description.hasAttribute("lang"))
                ui->descriptionEditLanguageList->addItem("en");
            else
                ui->descriptionEditLanguageList->addItem(description.attribute("lang"));
            description = description.nextSiblingElement("description");
        }
    }
    if(!skills[selectedItem].hasAttribute("type"))
        ui->type->setText(QString());
    else
        ui->type->setText(skills[selectedItem].attribute("type"));
    ui->stackedWidget->setCurrentWidget(ui->page_edit);
    ui->tabWidget->setCurrentWidget(ui->tabGeneral);
    loadingTheInformations=false;
    on_nameEditLanguageList_currentIndexChanged(ui->nameEditLanguageList->currentIndex());
    on_descriptionEditLanguageList_currentIndexChanged(ui->descriptionEditLanguageList->currentIndex());
}

void MainWindow::updateLevelList()
{
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    //load level
    {
        bool ok;
        ui->level->clear();
        QDomElement effect = skills[selectedItem].firstChildElement("effect");
        if(!effect.isNull() && effect.isElement())
        {
            QSet<quint32> levelNumberList;
            QDomElement level = effect.firstChildElement("level");
            while(!level.isNull())
            {
                if(!level.hasAttribute("number"))
                    level.parentNode().removeChild(level);
                else
                {
                    quint32 level_number=level.attribute("number").toUInt(&ok);
                    if(ok)
                    {
                        if(levelNumberList.contains(level_number))
                            level.parentNode().removeChild(level);
                        else
                        {
                            levelNumberList << level_number;
                            QListWidgetItem *item=new QListWidgetItem(ui->level);
                            item->setText(tr("Level %1").arg(level_number));
                            item->setData(99,level_number);
                            ui->level->addItem(item);
                        }
                    }
                    else
                        level.parentNode().removeChild(level);
                }
                level = level.nextSiblingElement("level");
            }
        }
    }
}

void MainWindow::on_itemListAdd_clicked()
{
    int index=1;
    while(skills.contains(index))
        index++;
    bool ok;
    index=QInputDialog::getInt(this,tr("Id"),tr("What id do you want"),index,0,2000000000,1,&ok);
    if(!ok)
        return;
    if(skills.contains(index))
    {
        QMessageBox::warning(this,tr("Error"),tr("Id already taken"));
        return;
    }
    QDomElement newXmlElement=domDocument.createElement("skill");
    newXmlElement.setAttribute("id",index);
    domDocument.documentElement().appendChild(newXmlElement);
    skills[index]=newXmlElement;
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
    if(!skills.contains(id))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable remove the bot, because the returned id is not into the list"));
        return;
    }
    bool ok;
    //load the bots
    QDomElement child = domDocument.documentElement().firstChildElement("skill");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id"))
            qDebug() << QStringLiteral("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
        else if(!child.isElement())
            qDebug() << QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
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
                qDebug() << QStringLiteral("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
        }
        child = child.nextSiblingElement("skill");
    }
    skills.remove(id);
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
    if(ui->nameEditLanguageList->count()<=0)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    Q_UNUSED(index);
    loadingTheInformations=true;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    QDomElement name = skills[selectedItem].firstChildElement("name");
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
    if(ui->descriptionEditLanguageList->count()<=0)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    Q_UNUSED(index);
    loadingTheInformations=true;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    QDomElement description = skills[selectedItem].firstChildElement("description");
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
    QDomElement name = skills[selectedItem].firstChildElement("name");
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
    QDomElement description = skills[selectedItem].firstChildElement("description");
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

/*void MainWindow::on_price_editingFinished()
{
    if(loadingTheInformations)
        return;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    skills[selectedItem].setAttribute("price",ui->price->value());
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
    skills[selectedItem].setAttribute("image",image);
}*/

void MainWindow::on_nameEditLanguageAdd_clicked()
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
    QDomElement newXmlElement=domDocument.createElement("name");
    newXmlElement.setAttribute("lang",lang);
    QDomText newTextElement=domDocument.createTextNode(name);
    newXmlElement.appendChild(newTextElement);
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    skills[selectedItem].appendChild(newXmlElement);
    ui->nameEditLanguageList->addItem(lang);
    ui->nameEditLanguageList->setCurrentIndex(ui->nameEditLanguageList->count()-1);
}

void MainWindow::on_nameEditLanguageRemove_clicked()
{
    QString selectedLang=ui->nameEditLanguageList->currentText();
    QDomElement child = domDocument.documentElement().firstChildElement("name");
    while(!child.isNull())
    {
        QString lang="en";
        if(child.hasAttribute("lang"))
            lang=child.attribute("lang");
        if(selectedLang==lang)
        {
            child.parentNode().removeChild(child);
            break;
        }
        child = child.nextSiblingElement("name");
    }
    ui->nameEditLanguageList->removeItem(ui->nameEditLanguageList->currentIndex());
}

void MainWindow::on_descriptionEditLanguageAdd_clicked()
{
    QString lang=QInputDialog::getText(this,tr("Language"),tr("Give the language code"));
    if(lang.isEmpty())
        return;
    QString description=QInputDialog::getText(this,tr("Description"),tr("Give the description for this language"));
    if(description.isEmpty())
        return;
    QListWidgetItem * item=new QListWidgetItem();
    item->setData(99,lang);
    item->setData(98,description);
    item->setText(QStringLiteral("%1: %2").arg(lang).arg(description));
    QDomElement newXmlElement=domDocument.createElement("description");
    newXmlElement.setAttribute("lang",lang);
    QDomText newTextElement=domDocument.createTextNode(description);
    newXmlElement.appendChild(newTextElement);
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    skills[selectedItem].appendChild(newXmlElement);
    ui->descriptionEditLanguageList->addItem(lang);
    ui->descriptionEditLanguageList->setCurrentIndex(ui->descriptionEditLanguageList->count()-1);
}

void MainWindow::on_descriptionEditLanguageRemove_clicked()
{
    QString selectedLang=ui->descriptionEditLanguageList->currentText();
    QDomElement child = domDocument.documentElement().firstChildElement("description");
    while(!child.isNull())
    {
        QString lang="en";
        if(child.hasAttribute("lang"))
            lang=child.attribute("lang");
        if(selectedLang==lang)
        {
            child.parentNode().removeChild(child);
            break;
        }
        child = child.nextSiblingElement("description");
    }
    ui->descriptionEditLanguageList->removeItem(ui->descriptionEditLanguageList->currentIndex());
}

void MainWindow::on_levelAdd_clicked()
{
    bool ok;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    if(!skills.contains(selectedItem))
        return;
    quint32 maxLevel=1;
    QDomElement effect = skills[selectedItem].firstChildElement("effect");
    if(!effect.isNull() && effect.isElement())
    {
        QDomElement level = effect.firstChildElement("level");
        while(!level.isNull())
        {
            if(!level.hasAttribute("number"))
                level.parentNode().removeChild(level);
            else
            {
                quint32 level_number=level.attribute("number").toUInt(&ok);
                if(ok)
                {
                    if(level_number>=maxLevel)
                        maxLevel=level_number+1;
                }
                else
                    level.parentNode().removeChild(level);
            }
            level = level.nextSiblingElement("level");
        }
    }
    else
    {
        effect=domDocument.createElement("effect");
        skills[selectedItem].appendChild(effect);
    }
    QDomElement newXmlElement=domDocument.createElement("level");
    newXmlElement.setAttribute("number",maxLevel);
    effect.appendChild(newXmlElement);
    ui->stackedWidget->setCurrentWidget(ui->page_level);
    updateLevelList();
}

void MainWindow::on_levelRemove_clicked()
{
    QList<QListWidgetItem *> itemsLevelUI=ui->level->selectedItems();
    if(itemsLevelUI.size()!=1)
        return;
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    currentLevelInfo.parentNode().removeChild(currentLevelInfo);
    updateLevelList();
}

void MainWindow::on_levelEdit_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->level->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_level_itemDoubleClicked(selectedItems.first());
}

void MainWindow::on_level_itemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    ui->stackedWidget->setCurrentWidget(ui->page_level);
    bool ok;
    const QDomElement &level=getCurrentLevelInfo();
    //number
    {
        if(level.hasAttribute("number"))
        {
            quint16 number=level.attribute("number").toUShort(&ok);
            if(ok)
                ui->level_level->setValue(number);
        }
        else
            ok=false;
        if(!ok)
            ui->level_level->setValue(1);
    }
    //sp
    {
        if(level.hasAttribute("sp"))
        {
            quint16 sp=level.attribute("sp").toUShort(&ok);
            if(ok)
                ui->level_sp->setValue(sp);
        }
        else
            ok=false;
        if(!ok)
            ui->level_sp->setValue(0);
    }
    //endurance
    {
        if(level.hasAttribute("endurance"))
        {
            quint16 endurance=level.attribute("endurance").toUShort(&ok);
            if(ok)
                ui->level_endurance->setValue(endurance);
        }
        else
            ok=false;
        if(!ok)
            ui->level_endurance->setValue(1);
    }
    updateBuffList();
    updateLifeList();
}

void MainWindow::updateBuffList()
{
    const QDomElement &level=getCurrentLevelInfo();
    //buff
    {
        ui->level_buff->clear();
        int index=0;
        QDomElement child = level.firstChildElement("buff");
        while(!child.isNull())
        {
            if(!child.hasAttribute("id"))
                qDebug() << QStringLiteral("Has not attribute \"quantity\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
            else if(!child.isElement())
                qDebug() << QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
            else
            {
                int success=100;
                if(child.hasAttribute("success"))
                {
                    bool ok;
                    QString successText=child.attribute("success");
                    successText.remove("%");
                    success=successText.toUShort(&ok);
                    if(!ok)
                    {
                        qDebug() << QStringLiteral("success not a number: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
                        success=100;
                    }
                    else if(success<=0 || success>100)
                    {
                        qDebug() << QStringLiteral("success not in range >0 && <=100: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
                        success=100;
                    }
                }
                int level=1;
                if(child.hasAttribute("level"))
                {
                    bool ok;
                    level=child.attribute("level").toUShort(&ok);
                    if(!ok)
                    {
                        qDebug() << QStringLiteral("level not a number: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
                        level=1;
                    }
                }
                QString applyOn="aloneEnemy";
                if(child.hasAttribute("applyOn"))
                {
                    applyOn=child.attribute("applyOn");
                    if(applyOn=="aloneEnemy")
                        child.removeAttribute("applyOn");
                    else if(applyOn=="themself")
                    {}
                    else if(applyOn=="allEnemy")
                    {}
                    else if(applyOn=="allAlly")
                    {}
                    else
                    {
                        child.removeAttribute("applyOn");
                        qDebug() << QStringLiteral("Attribute \"applyOn\" wrong: %3 child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()).arg(applyOn);
                        applyOn="aloneEnemy";
                    }
                }
                QListWidgetItem *item=new QListWidgetItem();
                if(level==1)
                    item->setText(tr("Buff id %1 on %2").arg(child.attribute("id")).arg(applyOn));
                else
                    item->setText(tr("Buff id %1 level %3 on %2").arg(child.attribute("id")).arg(applyOn).arg(level));
                if(success!=100)
                    item->setText(" ("+tr("success: %1%").arg(success)+")");
                item->setData(99,index);
                ui->level_buff->addItem(item);
            }
            index++;
            child = child.nextSiblingElement("buff");
        }
    }
}

void MainWindow::updateLifeList()
{
    const QDomElement &level=getCurrentLevelInfo();
    //life
    {
        ui->level_life->clear();
        int index=0;
        QDomElement child = level.firstChildElement("life");
        while(!child.isNull())
        {
            if(!child.hasAttribute("quantity"))
                qDebug() << QStringLiteral("Has not attribute \"quantity\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
            else if(!child.isElement())
                qDebug() << QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
            else
            {
                int success=100;
                if(child.hasAttribute("success"))
                {
                    bool ok;
                    QString successText=child.attribute("success");
                    successText.remove("%");
                    success=successText.toUShort(&ok);
                    if(!ok)
                    {
                        qDebug() << QStringLiteral("success not a number: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
                        success=100;
                    }
                    else if(success<=0 || success>100)
                    {
                        qDebug() << QStringLiteral("success not in range >0 && <=100: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
                        success=100;
                    }
                }
                QString applyOn="aloneEnemy";
                if(child.hasAttribute("applyOn"))
                {
                    applyOn=child.attribute("applyOn");
                    if(applyOn=="aloneEnemy")
                        child.removeAttribute("applyOn");
                    else if(applyOn=="themself")
                    {}
                    else if(applyOn=="allEnemy")
                    {}
                    else if(applyOn=="allAlly")
                    {}
                    else
                    {
                        child.removeAttribute("applyOn");
                        qDebug() << QStringLiteral("Attribute \"applyOn\" wrong: %3 child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()).arg(applyOn);
                        applyOn="aloneEnemy";
                    }
                }
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1 on %2").arg(child.attribute("quantity")).arg(applyOn));
                if(success!=100)
                    item->setText(" ("+tr("success: %1%").arg(success)+")");
                item->setData(99,index);
                ui->level_life->addItem(item);
            }
            index++;
            child = child.nextSiblingElement("life");
        }
    }
}

QDomElement MainWindow::getCurrentLevelInfo()
{
    bool ok;
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return QDomElement();
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    if(!skills.contains(selectedItem))
        return QDomElement();
    QList<QListWidgetItem *> itemsLevelUI=ui->level->selectedItems();
    if(itemsLevelUI.size()!=1)
        return QDomElement();
    quint32 selectedLevel=itemsLevelUI.first()->data(99).toUInt();
    QDomElement effect = skills[selectedItem].firstChildElement("effect");
    if(!effect.isNull() && effect.isElement())
    {
        QDomElement level = effect.firstChildElement("level");
        while(!level.isNull())
        {
            if(!level.hasAttribute("number"))
                level.parentNode().removeChild(level);
            else
            {
                quint32 level_number=level.attribute("number").toUInt(&ok);
                if(ok)
                {
                    if(level_number==selectedLevel)
                        return level;
                }
                else
                    level.parentNode().removeChild(level);
            }
            level = level.nextSiblingElement("level");
        }
    }
    else
        return QDomElement();
    return QDomElement();
}

QDomElement MainWindow::getCurrentBuffInfo()
{
    QList<QListWidgetItem *> selectedItems=ui->level_buff->selectedItems();
    if(selectedItems.size()!=1)
        return QDomElement();
    const int &selectedIndex=selectedItems.first()->data(99).toUInt();
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    QDomElement buff = currentLevelInfo.firstChildElement("buff");
    int index=0;
    while(!buff.isNull())
    {
        if(index==selectedIndex)
            return buff;
        index++;
        buff = buff.nextSiblingElement("buff");
    }
    return QDomElement();
}

QDomElement MainWindow::getCurrentLifeInfo()
{
    QList<QListWidgetItem *> selectedItems=ui->level_life->selectedItems();
    if(selectedItems.size()!=1)
        return QDomElement();
    const int &selectedIndex=selectedItems.first()->data(99).toUInt();
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    QDomElement life = currentLevelInfo.firstChildElement("life");
    int index=0;
    while(!life.isNull())
    {
        if(index==selectedIndex)
            return life;
        index++;
        life = life.nextSiblingElement("life");
    }
    return QDomElement();
}

void MainWindow::on_skillLevelBack_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_edit);
}

void MainWindow::on_life_back_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_level);
}

void MainWindow::on_buff_back_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_level);
}

void MainWindow::on_skillLifeEffectEdit_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->level_life->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_level_life_itemDoubleClicked(selectedItems.first());
}

void MainWindow::on_level_life_itemDoubleClicked(QListWidgetItem *item)
{
    ui->stackedWidget->setCurrentWidget(ui->page_life);
    bool ok;
    Q_UNUSED(item);
    QDomElement currentLifeInfo=getCurrentLifeInfo();
    //quantity
    {
        QString quantityText;
        if(!currentLifeInfo.hasAttribute("quantity"))
            quantityText="0";
        else
            quantityText=currentLifeInfo.attribute("quantity");
        quantityText.remove("+");
        if(quantityText.contains("%"))
        {
            quantityText.remove("%");
            ui->life_quantity_type->setCurrentIndex(1);
        }
        else
            ui->life_quantity_type->setCurrentIndex(0);
        ui->life_quantity->setValue(quantityText.toInt(&ok));
        if(!ok)
            ui->life_quantity->setValue(0);
    }
    //success
    {
        QString successText;
        if(!currentLifeInfo.hasAttribute("success"))
            successText="100%";
        else
            successText=currentLifeInfo.attribute("success");
        successText.remove("%");
        ui->life_success->setValue(successText.toUShort(&ok));
        if(!ok)
            ui->life_success->setValue(0);
    }
    //life_apply_on
    {
        QString applyOnText;
        if(!currentLifeInfo.hasAttribute("applyOn"))
            applyOnText="aloneEnemy";
        else
            applyOnText=currentLifeInfo.attribute("applyOn");
        if(applyOnText=="aloneEnemy")
            ui->life_apply_on->setCurrentIndex(0);
        else if(applyOnText=="themself")
            ui->life_apply_on->setCurrentIndex(1);
        else if(applyOnText=="allEnemy")
            ui->life_apply_on->setCurrentIndex(2);
        else if(applyOnText=="allAlly")
            ui->life_apply_on->setCurrentIndex(3);
        else
            ui->life_apply_on->setCurrentIndex(0);
    }
}

void MainWindow::on_skillBuffEffectEdit_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->level_buff->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_level_buff_itemDoubleClicked(selectedItems.first());
}

void MainWindow::on_level_buff_itemDoubleClicked(QListWidgetItem *item)
{
    ui->stackedWidget->setCurrentWidget(ui->page_buff);
    bool ok;
    Q_UNUSED(item);
    QDomElement currentBuffInfo=getCurrentBuffInfo();
    //id
    {
        QString idText;
        if(!currentBuffInfo.hasAttribute("id"))
            idText="1";
        else
            idText=currentBuffInfo.attribute("id");
        ui->buff_id->setValue(idText.toUInt(&ok));
        if(!ok)
            ui->buff_id->setValue(1);
    }
    //level
    {
        QString idText;
        if(!currentBuffInfo.hasAttribute("level"))
            idText="1";
        else
            idText=currentBuffInfo.attribute("level");
        ui->buff_level->setValue(idText.toUInt(&ok));
        if(!ok)
            ui->buff_level->setValue(1);
    }
    //success
    {
        QString successText;
        if(!currentBuffInfo.hasAttribute("success"))
            successText="100%";
        else
            successText=currentBuffInfo.attribute("success");
        successText.remove("%");
        ui->buff_success->setValue(successText.toUShort(&ok));
        if(!ok)
            ui->buff_success->setValue(0);
    }
    //life_apply_on
    {
        QString applyOnText;
        if(!currentBuffInfo.hasAttribute("applyOn"))
            applyOnText="aloneEnemy";
        else
            applyOnText=currentBuffInfo.attribute("applyOn");
        if(applyOnText=="aloneEnemy")
            ui->life_apply_on->setCurrentIndex(0);
        else if(applyOnText=="themself")
            ui->life_apply_on->setCurrentIndex(1);
        else if(applyOnText=="allEnemy")
            ui->life_apply_on->setCurrentIndex(2);
        else if(applyOnText=="allAlly")
            ui->life_apply_on->setCurrentIndex(3);
        else
            ui->life_apply_on->setCurrentIndex(0);
    }
}

void MainWindow::on_life_quantity_editingFinished()
{
    QDomElement currentLifeInfo=getCurrentLifeInfo();
    if(ui->life_quantity_type->currentIndex()==0)
        currentLifeInfo.setAttribute("quantity",ui->life_quantity->value());
    else
        currentLifeInfo.setAttribute("quantity",QStringLiteral("%1%").arg(ui->life_quantity->value()));
}

void MainWindow::on_life_quantity_type_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    QDomElement currentLifeInfo=getCurrentLifeInfo();
    if(ui->life_quantity_type->currentIndex()==0)
        currentLifeInfo.setAttribute("quantity",ui->life_quantity->value());
    else
        currentLifeInfo.setAttribute("quantity",QStringLiteral("%1%").arg(ui->life_quantity->value()));
}

void MainWindow::on_life_success_editingFinished()
{
    QDomElement currentLifeInfo=getCurrentLifeInfo();
    if(ui->life_success->value()==100)
        currentLifeInfo.removeAttribute("success");
    else
        currentLifeInfo.setAttribute("success",QStringLiteral("%1%").arg(ui->life_success->value()));
}

void MainWindow::on_life_apply_on_currentIndexChanged(int index)
{
    QDomElement currentLifeInfo=getCurrentLifeInfo();
    switch(index)
    {
        default:
        case 0:
            currentLifeInfo.removeAttribute("applyOn");
        break;
        case 1:
            currentLifeInfo.setAttribute("applyOn","themself");
        break;
        case 2:
            currentLifeInfo.setAttribute("applyOn","allEnemy");
        break;
        case 3:
            currentLifeInfo.setAttribute("applyOn","allAlly");
        break;
    }
}

void MainWindow::on_buff_id_editingFinished()
{
    QDomElement currentBuffInfo=getCurrentBuffInfo();
    currentBuffInfo.setAttribute("id",ui->buff_id->value());
}

void MainWindow::on_buff_level_editingFinished()
{
    QDomElement currentBuffInfo=getCurrentBuffInfo();
    if(ui->buff_level->value()==1)
        currentBuffInfo.removeAttribute("level");
    else
        currentBuffInfo.setAttribute("level",ui->buff_level->value());
}

void MainWindow::on_buff_success_editingFinished()
{
    QDomElement currentBuffInfo=getCurrentBuffInfo();
    if(ui->buff_success->value()==100)
        currentBuffInfo.removeAttribute("success");
    else
        currentBuffInfo.setAttribute("success",QStringLiteral("%1%").arg(ui->buff_success->value()));
}

void MainWindow::on_buff_apply_on_currentIndexChanged(int index)
{
    QDomElement currentBuffInfo=getCurrentBuffInfo();
    switch(index)
    {
        default:
        case 0:
            currentBuffInfo.removeAttribute("applyOn");
        break;
        case 1:
            currentBuffInfo.setAttribute("applyOn","themself");
        break;
        case 2:
            currentBuffInfo.setAttribute("applyOn","allEnemy");
        break;
        case 3:
            currentBuffInfo.setAttribute("applyOn","allAlly");
        break;
    }
}

void MainWindow::on_level_level_editingFinished()
{
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    bool ok;
    quint32 oldNumber=currentLevelInfo.attribute("number").toUInt(&ok);
    if(!ok)
        return;
    int index=0;
    while(index<ui->level->count())
    {
        const QListWidgetItem *item=ui->level->item(index);
        if(item->data(99).toInt()==ui->level_level->value() && !item->isSelected() && oldNumber!=0)
        {
            //QMessageBox::warning(this,tr("Error"),tr("Sorry, but another level with same number"));
            ui->level_level->setValue(oldNumber);
            return;
        }
        index++;
    }
    currentLevelInfo.setAttribute("number",ui->level_level->value());
}

void MainWindow::on_level_sp_editingFinished()
{
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    currentLevelInfo.setAttribute("sp",ui->level_sp->value());
}

void MainWindow::on_level_endurance_editingFinished()
{
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    currentLevelInfo.setAttribute("endurance",ui->level_endurance->value());
}

void MainWindow::on_skillLifeEffectAdd_clicked()
{
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    QDomElement newXmlElement=domDocument.createElement("life");
    newXmlElement.setAttribute("quantity",1);
    currentLevelInfo.appendChild(newXmlElement);
    updateLifeList();
}

void MainWindow::on_skillLifeEffectDelete_clicked()
{
    QList<QListWidgetItem *> itemsLevelUI=ui->level_life->selectedItems();
    if(itemsLevelUI.size()!=1)
        return;
    QDomElement currentLifeInfo=getCurrentLifeInfo();
    currentLifeInfo.parentNode().removeChild(currentLifeInfo);
    updateLifeList();
}

void MainWindow::on_skillBuffEffectAdd_clicked()
{
    QDomElement currentLevelInfo=getCurrentLevelInfo();
    QDomElement newXmlElement=domDocument.createElement("buff");
    newXmlElement.setAttribute("id",1);
    currentLevelInfo.appendChild(newXmlElement);
    updateBuffList();
}

void MainWindow::on_skillBuffEffectDelete_clicked()
{
    QList<QListWidgetItem *> itemsLevelUI=ui->level_buff->selectedItems();
    if(itemsLevelUI.size()!=1)
        return;
    QDomElement currentBuffInfo=getCurrentBuffInfo();
    currentBuffInfo.parentNode().removeChild(currentBuffInfo);
    updateBuffList();
}

void MainWindow::on_type_editingFinished()
{
    QList<QListWidgetItem *> itemsUI=ui->itemList->selectedItems();
    if(itemsUI.size()!=1)
        return;
    quint32 selectedItem=itemsUI.first()->data(99).toUInt();
    if(ui->type->text().isEmpty())
        skills[selectedItem].removeAttribute("type");
    else
        skills[selectedItem].setAttribute("type",ui->type->text());
}
