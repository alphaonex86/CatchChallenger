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

#include "StepType.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings("CatchChallenger","bot-editor",parent)
{
    ui->setupUi(this);
    ui->lineEditBotFile->setText(settings.value("file").toString());
    if(ui->lineEditBotFile->text().isEmpty())
        ui->browseBotFile->setFocus();
    else
        ui->openBotFile->setFocus();
    ui->stackedWidget->setCurrentWidget(ui->page_welcome);
    updateType();
    connect(ui->stepEditLanguageList,SIGNAL(currentIndexChanged(int)),this,SLOT(updateTextDisplayed()),Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateType()
{
    allowedType["text"]=tr("Text");
    allowedType["shop"]=tr("Shop");
    allowedType["sell"]=tr("Sell");
    allowedType["quests"]=tr("Quests");
    allowedType["warehouse"]=tr("Warehouse");
    allowedType["learn"]=tr("Learn");
    allowedType["heal"]=tr("Heal");
    allowedType["fight"]=tr("Fight");
    allowedType["clan"]=tr("Clan");
    allowedType["zonecapture"]=tr("Zone capture");
    allowedType["industry"]=tr("Industry");
    allowedType["market"]=tr("Market");

    reverseAllowedType.clear();
    QHash<QString,QString>::const_iterator i = allowedType.constBegin();
    while (i != allowedType.constEnd()) {
        reverseAllowedType[i.value()]=i.key();
        ++i;
    }
}

void MainWindow::on_browseBotFile_clicked()
{
    QString file=QFileDialog::getOpenFileName(this,"Open the bot file",QString(),tr("Bot file")+" (*.xml)");
    if(!file.isEmpty())
        ui->lineEditBotFile->setText(file);
}

void MainWindow::on_openBotFile_clicked()
{
    if(ui->lineEditBotFile->text().isEmpty())
    {
        QMessageBox::warning(this,tr("Error"),tr("You need select a bot file"));
        return;
    }
    if(!ui->lineEditBotFile->text().contains(QRegExp("\\.xml$")))
    {
        QMessageBox::warning(this,tr("Error"),tr("The bot file is a xml file"));
        return;
    }
    QFile xmlFile(ui->lineEditBotFile->text());
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    settings.setValue("file",ui->lineEditBotFile->text());
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
    if(root.tagName()!="bots")
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\nThe root balise is not bots").arg(xmlFile.fileName()));
        return;
    }
    bool error=false;
    //load the bots
    QDomElement child = root.firstChildElement("bot");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id"))
        {
            qDebug() << QString("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
            error=true;
        }
        else if(!child.isElement())
        {
            qDebug() << QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber()));
            error=true;
        }
        else
        {
            quint8 id=child.attribute("id").toUShort(&ok);
            if(ok)
            {
                QDomElement step = child.firstChildElement("step");
                while(!step.isNull())
                {
                    if(!step.hasAttribute("id"))
                    {
                        qDebug() << QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber());
                        error=true;
                    }
                    else if(!step.hasAttribute("type"))
                    {
                        qDebug() << QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber());
                        error=true;
                    }
                    else if(!step.isElement())
                    {
                        qDebug() << QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute("type")).arg(step.lineNumber()));
                        error=true;
                    }
                    else
                    {
                        quint8 stepId=step.attribute("id").toUShort(&ok);
                        if(ok)
                        {
                            botFiles[id].step[stepId]=step;
                            botFiles[id].botId=id;
                        }
                    }
                    step = step.nextSiblingElement("step");
                }
            }
            else
            {
                qDebug() << QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
                error=true;
            }
        }
        child = child.nextSiblingElement("bot");
    }
    if(error)
        QMessageBox::warning(this,tr("Error"),tr("Some error have been found into the file").arg(xmlFile.fileName()));
    updateBotList();
    ui->stackedWidget->setCurrentWidget(ui->page_bot_list);
}

void MainWindow::updateBotList()
{
    ui->botList->clear();
    QHash<quint8,Bot>::const_iterator i = botFiles.constBegin();
    while (i != botFiles.constEnd()) {
        QListWidgetItem *item=new QListWidgetItem();
        if(i.value().step.contains(1))
            item->setText(tr("Bot %1").arg(i.key()));
        else
            item->setText(tr("Bot %1 (have not step 1)").arg(i.key()));
        item->setData(99,i.key());
        ui->botList->addItem(item);
        ++i;
    }
}

void MainWindow::on_botListAdd_clicked()
{
    int index=1;
    while(botFiles.contains(index))
        index++;
    bool ok;
    int id=QInputDialog::getInt(this,tr("Id of the bot"),tr("Give the id for the new bot"),index,index,2147483647,1,&ok);
    if(botFiles.contains(id))
    {
        QMessageBox::warning(this,tr("Error"),tr("Sorry but this id is already taken"));
        return;
    }
    Bot tempBot;
    tempBot.botId=id;
    botFiles[id]=tempBot;
    QDomElement newXmlElement=domDocument.createElement("bot");
    newXmlElement.setAttribute("id",id);
    domDocument.documentElement().appendChild(newXmlElement);
    updateBotList();
}

void MainWindow::on_botListDelete_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->botList->selectedItems();
    if(selectedItems.size()!=1)
    {
        QMessageBox::warning(this,tr("Error"),tr("Select a bot into the bot list"));
        return;
    }
    quint8 id=selectedItems.first()->data(99).toUInt();
    if(!botFiles.contains(id))
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
    botFiles.remove(id);
    updateBotList();
}

void MainWindow::on_botListEdit_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->botList->selectedItems();
    if(selectedItems.size()!=1)
    {
        QMessageBox::warning(this,tr("Error"),tr("Select a bot into the bot list"));
        return;
    }
    on_botList_itemDoubleClicked(selectedItems.first());
}

void MainWindow::updateStepList()
{
    ui->stepList->clear();
    QHash<quint8,QDomElement>::const_iterator i = botFiles[selectedBot].step.constBegin();
    while (i != botFiles[selectedBot].step.constEnd()) {
        QListWidgetItem *item=new QListWidgetItem();
        if(i.value().hasAttribute("type"))
        {
            if(!allowedType.contains(i.value().attribute("type")))
                item->setText(tr("Step %1 with type wrong: %2").arg(i.key()).arg(i.value().attribute("type")));
            else
                item->setText(tr("Step %1: %2").arg(i.key()).arg(allowedType[i.value().attribute("type")]));
        }
        else
            item->setText(tr("Step %1 (have not the attribute type)").arg(i.key()));
        item->setData(99,i.key());
        ui->stepList->addItem(item);
        ++i;
    }
}

void MainWindow::on_stepListAdd_clicked()
{
    int index=1;
    while(botFiles[selectedBot].step.contains(index))
        index++;
    bool ok;
    int id=QInputDialog::getInt(this,tr("Id of the step"),tr("Give the id for the new step"),index,index,2147483647,1,&ok);
    if(botFiles[selectedBot].step.contains(id))
    {
        QMessageBox::warning(this,tr("Error"),tr("Sorry but this id is already taken"));
        return;
    }
    botFiles[selectedBot].step[id]=domDocument.createElement("step");
    botFiles[selectedBot].step[id].setAttribute("id",id);
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
                if(tempId==selectedBot)
                {
                    child.appendChild(botFiles[selectedBot].step[id]);
                    break;
                }
            }
            else
                qDebug() << QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber());
        }
        child = child.nextSiblingElement("bot");
    }
    //load the edit
    editStep(id,true);
    updateStepList();
    if(id==1)
    {
        updateBotList();
        ui->stepListTitle->setText(tr("Step list for the bot: %1").arg(selectedBot));
    }
}

void MainWindow::on_stepListDelete_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->stepList->selectedItems();
    if(selectedItems.size()!=1)
    {
        QMessageBox::warning(this,tr("Error"),tr("Select a step into the step list"));
        return;
    }
    quint8 id=selectedItems.first()->data(99).toUInt();
    if(!botFiles[selectedBot].step.contains(id))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable remove the step, because the returned id is not into the list"));
        return;
    }
    botFiles[selectedBot].step[id].parentNode().removeChild(botFiles[selectedBot].step[id]);
    botFiles[selectedBot].step.remove(id);
    updateStepList();
    if(id==1)
        updateBotList();
}

void MainWindow::on_stepListEdit_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->stepList->selectedItems();
    if(selectedItems.size()!=1)
    {
        QMessageBox::warning(this,tr("Error"),tr("Select a step into the bot list"));
        return;
    }
    on_stepList_itemDoubleClicked(selectedItems.first());
}

void MainWindow::on_stepListBack_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_bot_list);
}

void MainWindow::on_stepEditBack_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_step_list);
}

void MainWindow::editStep(quint8 id,bool newStep)
{
    selectedStep=id;
    QDomElement step=botFiles[selectedBot].step[selectedStep];
    bool needType=false;
    if(!step.hasAttribute("type"))
        needType=true;
    else if(!allowedType.contains(step.attribute("type")))
        needType=true;
    if(needType)
    {
        StepType stepType(allowedType,this);
        stepType.exec();
        if(!stepType.validated())
        {
            ui->stackedWidget->setCurrentWidget(ui->page_step_list);
            return;
        }
        step.setAttribute("type",stepType.type());
    }
    QString type=step.attribute("type");
    int index=0;
    while(index<ui->tabWidget->count())
    {
        if(allowedType[type]==ui->tabWidget->tabText(index))
        {
            ui->tabWidget->setTabEnabled(index,true);
            ui->tabWidget->setCurrentIndex(index);
        }
        else
            ui->tabWidget->setTabEnabled(index,false);
        index++;
    }
    bool error=false;
    if(type=="text")
    {
        ui->stepEditLanguageList->clear();
        QDomElement text = step.firstChildElement("text");
        while(!text.isNull())
        {
            if(!text.hasAttribute("lang"))
            {
                qDebug() << QString("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(text.tagName()).arg(text.lineNumber());
                error=true;
            }
            else if(!text.isElement())
            {
                qDebug() << QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(text.tagName().arg(text.attribute("name")).arg(text.lineNumber()));
                error=true;
            }
            else
                ui->stepEditLanguageList->addItem(text.attribute("lang"));
            text = text.nextSiblingElement("text");
        }
        updateTextDisplayed();
    }
    else if(type=="shop")
    {
        bool ok;
        quint32 id=step.attribute("shop").toUInt(&ok);
        if(!ok)
        {
            if(newStep)
                step.setAttribute("shop",1);
            ui->stepEditShop->setValue(1);
            error=true;
        }
        else
            ui->stepEditShop->setValue(id);
    }
    else if(type=="sell")
    {
        bool ok;
        quint32 id=step.attribute("shop").toUInt(&ok);
        if(!ok)
        {
            if(newStep)
                step.setAttribute("shop",1);
            ui->stepEditSell->setValue(1);
            error=true;
        }
        else
            ui->stepEditSell->setValue(id);
    }
    else if(type=="fight")
    {
        bool ok;
        quint32 id=step.attribute("fightid").toUInt(&ok);
        if(!ok)
        {
            if(newStep)
                step.setAttribute("fightid",1);
            ui->stepEditFight->setValue(1);
            error=true;
        }
        else
            ui->stepEditFight->setValue(id);
    }
    else if(type=="zonecapture")
    {
        if(!step.hasAttribute("zone"))
        {
            if(newStep)
                step.setAttribute("zone","");
            ui->stepEditZoneCapture->setText(QString());
            error=true;
        }
        else
            ui->stepEditZoneCapture->setText(step.attribute("zone"));
    }
    else if(type=="industry")
    {
        bool ok;
        quint32 id=step.attribute("factory_id").toUInt(&ok);
        if(!ok)
        {
            if(newStep)
                step.setAttribute("factory_id",1);
            ui->stepEditIndustry->setValue(1);
            error=true;
        }
        else
            ui->stepEditIndustry->setValue(id);
    }
    if(error && !newStep)
    {
        QMessageBox::warning(this,tr("Error"),tr("Error during loading the step"));
        return;
    }
    ui->stackedWidget->setCurrentWidget(ui->page_step_edit);
}

void MainWindow::updateTextDisplayed()
{
    bool found=false;
    QDomElement step=botFiles[selectedBot].step[selectedStep];
    QDomElement text = step.firstChildElement("text");
    while(!text.isNull())
    {
        if(text.hasAttribute("lang") && text.isElement() && text.attribute("lang")==ui->stepEditLanguageList->currentText())
        {
            ui->plainTextEdit->setPlainText(text.text());
            found=true;
            break;
        }
        text = text.nextSiblingElement("text");
    }
    if(!found)
        ui->plainTextEdit->setPlainText(QString());
}

void MainWindow::on_plainTextEdit_textChanged()
{
    QDomElement step=botFiles[selectedBot].step[selectedStep];
    QDomElement text = step.firstChildElement("text");
    while(!text.isNull())
    {
        if(text.hasAttribute("lang") && text.isElement() && text.attribute("lang")==ui->stepEditLanguageList->currentText())
        {
            QDomText newTextElement=text.ownerDocument().createTextNode(ui->plainTextEdit->toPlainText());
            int sub_index=0;
            QDomNodeList nodeList=text.childNodes();
            while(sub_index<nodeList.size())
            {
                text.removeChild(nodeList.at(sub_index));
                sub_index++;
            }
            text.appendChild(newTextElement);
            return;
        }
        text = text.nextSiblingElement("text");
    }
}

void MainWindow::on_stepEditLanguageRemove_clicked()
{
    QDomElement text = botFiles[selectedBot].step[selectedStep].firstChildElement("text");
    while(!text.isNull())
    {
        if(text.hasAttribute("lang") && text.isElement() && text.attribute("lang")==ui->stepEditLanguageList->currentText())
        {
            ui->stepEditLanguageList->removeItem(ui->stepEditLanguageList->currentIndex());
            botFiles[selectedBot].step[selectedStep].removeChild(text);
            updateTextDisplayed();
            return;
        }
        text = text.nextSiblingElement("text");
    }
}

void MainWindow::on_stepEditLanguageAdd_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("The country code"),
                                         tr("The country code (ISO, two letter):"), QLineEdit::Normal,
                                         "en", &ok);
    if (!ok || text.isEmpty())
        return;
    QDomElement textBalise = botFiles[selectedBot].step[selectedStep].firstChildElement("text");
    while(!textBalise.isNull())
    {
        if(textBalise.hasAttribute("lang") && textBalise.isElement() && textBalise.attribute("lang")==text)
        {
            QMessageBox::warning(this,tr("Error"),tr("This country letter is already found"));
            return;
        }
        textBalise = textBalise.nextSiblingElement("text");
    }
    if(!text.contains(QRegExp("^[a-z]{2}(_[A-Z]{2})?$")))
    {
        QMessageBox::warning(this,tr("Error"),tr("The country code is ISO code, then 2 letters only"));
        return;
    }
    QDomElement newElement=botFiles[selectedBot].step[selectedStep].ownerDocument().createElement("text");
    newElement.setAttribute("lang",text);
    if(!newElement.isNull())
        botFiles[selectedBot].step[selectedStep].appendChild(newElement);
    ui->stepEditLanguageList->addItem(text);
    ui->stepEditLanguageList->setCurrentIndex(ui->stepEditLanguageList->count()-1);
}

void MainWindow::on_stepEditShop_editingFinished()
{
    botFiles[selectedBot].step[selectedStep].setAttribute("shop",ui->stepEditShop->value());
}

void MainWindow::on_stepEditSell_editingFinished()
{
    botFiles[selectedBot].step[selectedStep].setAttribute("shop",ui->stepEditSell->value());
}

void MainWindow::on_stepEditFight_editingFinished()
{
    botFiles[selectedBot].step[selectedStep].setAttribute("fightid",ui->stepEditFight->value());
}

void MainWindow::on_botFileSave_clicked()
{
    QFile xmlFile(ui->lineEditBotFile->text());
    if(!xmlFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlFile.write(domDocument.toByteArray(4));
    xmlFile.close();
    QMessageBox::information(this,tr("Saved"),tr("The file have been correctly saved"));
}

void MainWindow::on_botList_itemDoubleClicked(QListWidgetItem *item)
{
    selectedBot=item->data(99).toUInt();
    if(!botFiles.contains(selectedBot))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to select the bot, because the returned id is not into the list"));
        return;
    }
    ui->stepListTitle->setText(tr("Step list for the bot: %1").arg(item->text()));
    updateStepList();
    ui->stackedWidget->setCurrentWidget(ui->page_step_list);
}

void MainWindow::on_stepList_itemDoubleClicked(QListWidgetItem *item)
{
    quint8 id=item->data(99).toUInt();
    if(!botFiles[selectedBot].step.contains(id))
    {
        QMessageBox::warning(this,tr("Error"),tr("Unable to select the step, because the returned id is not into the list"));
        return;
    }
    editStep(id);
}

void MainWindow::on_stepEditZoneCapture_editingFinished()
{
    botFiles[selectedBot].step[selectedStep].setAttribute("zone",ui->stepEditZoneCapture->text());
}

void MainWindow::on_stepEditIndustry_editingFinished()
{
    botFiles[selectedBot].step[selectedStep].setAttribute("factory_id",ui->stepEditFight->value());
}
