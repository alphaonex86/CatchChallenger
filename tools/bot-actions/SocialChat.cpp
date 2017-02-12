#include "SocialChat.h"
#include "ui_SocialChat.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../general/base/ChatParsing.h"

#include <QListWidget>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

SocialChat * SocialChat::socialChat=NULL;

SocialChat::SocialChat() :
    ui(new Ui::SocialChat)
{
    ui->setupUi(this);
    const QString destination(QCoreApplication::applicationDirPath()+"/chat.db");
    QFile destinationFile(destination);
    if(!destinationFile.exists())
    {
        QFile::copy(":/chat.db",destination);
        destinationFile.setPermissions(destinationFile.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser);
    }

    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(destination);

    if (!database.open())
    {
        std::cerr << "Error: connection with database fail" << std::endl;
        abort();
    }

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    lastText[0];
}

void SocialChat::showEvent(QShowEvent * event)
{
    bool first=true;
    QHash<CatchChallenger::Api_protocol *,ActionsBotInterface::Player>::const_iterator i = ActionsBotInterface::clientList.constBegin();
    while (i != ActionsBotInterface::clientList.constEnd()) {
        ActionsBotInterface::Player &client=const_cast<ActionsBotInterface::Player &>(i.value());
        if(client.api->getCaracterSelected())
        {
            if(first==true)
            {
                connect(client.api,&CatchChallenger::Api_protocol::new_system_text,this,&SocialChat::new_system_text);
                connect(client.api,&CatchChallenger::Api_protocol::new_chat_text,this,&SocialChat::new_chat_text);
                first=false;
            }
            const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client.api->get_player_informations();
            const QString &qtpseudo=QString::fromStdString(player_private_and_public_informations.public_informations.pseudo);
            ui->bots->addItem(qtpseudo);
            pseudoToBot[qtpseudo]=&client;
        }
        ++i;
    }

    if(!event->spontaneous())
    {
        if(ui->bots->count()==1)
        {
            QListWidgetItem * item=ui->bots->itemAt(0,0);
            if(!item->isSelected())
            {
                item->setSelected(true);
                on_bots_itemSelectionChanged();
            }
            ui->groupBoxBot->setVisible(false);
        }
        std::cout << "QShowEvent: QEvent::Type: " << (int)event->type() << std::endl;
    }
    QWidget::showEvent(event);
}

SocialChat::~SocialChat()
{
    delete ui;
}

void SocialChat::on_bots_itemSelectionChanged()
{
    ui->groupBoxVisiblePlayers->setEnabled(true);
    ui->groupBoxChat->setEnabled(true);
    ui->groupBoxInformations->setEnabled(true);

    loadPlayerInformation();
}

void SocialChat::loadPlayerInformation()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    ActionsBotInterface::Player * client=pseudoToBot.value(pseudo);
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->api->get_player_informations();
    ui->labelPseudo->setText(pseudo);
    ui->chatSpecText->setPlaceholderText(tr("Your text as %1").arg(pseudo));
    ui->globalChatText->setPlaceholderText(tr("Your text as %1").arg(pseudo));

    //front image
    if(playerInformations.public_informations.skinId<DatapackClientLoader::datapackLoader.skins.size())
        ui->labelImageAvatar->setPixmap(getFrontSkin(playerInformations.public_informations.skinId));

    QSqlQuery query;
    query.prepare("SELECT * FROM note WHERE player = (:player)");
    query.bindValue(":player",pseudo);
    if(!query.exec())
        qDebug() << "note load error: " << query.lastError();
    else if(query.next())
        ui->note->setPlainText(query.value("text").toString());
}

QPixmap SocialChat::getFrontSkin(const QString &skinName)
{
    if(frontSkinCacheString.contains(skinName))
        return frontSkinCacheString.value(skinName);
    const QPixmap skin(getFrontSkinPath(skinName));
    if(!skin.isNull())
        frontSkinCacheString[skinName]=skin;
    else
        frontSkinCacheString[skinName]=QPixmap(":/images/player_default/front.png");
    return frontSkinCacheString.value(skinName);
}

QPixmap SocialChat::getFrontSkin(const uint32_t &skinId)
{
    if(skinId<(uint32_t)DatapackClientLoader::datapackLoader.skins.size())
        return getFrontSkin(DatapackClientLoader::datapackLoader.skins.at(skinId));
    else
        return getFrontSkin(QString());
}

QString SocialChat::getFrontSkinPath(const QString &skin)
{
    return getSkinPath(skin,"front");
}

QPixmap SocialChat::getBackSkin(const QString &skinName)
{
    if(backSkinCacheString.contains(skinName))
        return backSkinCacheString.value(skinName);
    const QPixmap skin(getBackSkinPath(skinName));
    if(!skin.isNull())
        backSkinCacheString[skinName]=skin;
    else
        backSkinCacheString[skinName]=QPixmap(":/images/player_default/back.png");
    return backSkinCacheString.value(skinName);
}

QPixmap SocialChat::getBackSkin(const uint32_t &skinId)
{
    if(skinId<(uint32_t)DatapackClientLoader::datapackLoader.skins.size())
        return getFrontSkin(DatapackClientLoader::datapackLoader.skins.at(skinId));
    else
        return getFrontSkin(QString());
}

QString SocialChat::getBackSkinPath(const QString &skin)
{
    return getSkinPath(skin,"back");
}

QPixmap SocialChat::getTrainerSkin(const QString &skinName)
{
    if(trainerSkinCacheString.contains(skinName))
        return trainerSkinCacheString.value(skinName);
    const QPixmap skin(getTrainerSkinPath(skinName));
    if(!skin.isNull())
        trainerSkinCacheString[skinName]=skin;
    else
        trainerSkinCacheString[skinName]=QPixmap(":/images/player_default/trainer.png");
    return trainerSkinCacheString.value(skinName);
}

QPixmap SocialChat::getTrainerSkin(const uint32_t &skinId)
{
    if(skinId<(uint32_t)DatapackClientLoader::datapackLoader.skins.size())
        return getTrainerSkin(DatapackClientLoader::datapackLoader.skins.at(skinId));
    else
        return getTrainerSkin(QString());
}

QString SocialChat::getTrainerSkinPath(const QString &skin)
{
    return getSkinPath(skin,"trainer");
}

QString SocialChat::getSkinPath(const QString &skinName,const QString &type)
{
    {
        QFileInfo pngFile(DatapackClientLoader::datapackLoader.getDatapackPath()+DATAPACK_BASE_PATH_SKIN+skinName+QStringLiteral("/%1.png").arg(type));
        if(pngFile.exists())
            return pngFile.absoluteFilePath();
    }
    {
        QFileInfo gifFile(DatapackClientLoader::datapackLoader.getDatapackPath()+DATAPACK_BASE_PATH_SKIN+skinName+QStringLiteral("/%1.gif").arg(type));
        if(gifFile.exists())
            return gifFile.absoluteFilePath();
    }
    QDir folderList(DatapackClientLoader::datapackLoader.getDatapackPath()+DATAPACK_BASE_PATH_SKINBASE);
    const QStringList &entryList=folderList.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    int entryListIndex=0;
    while(entryListIndex<entryList.size())
    {
        {
            QFileInfo pngFile(QStringLiteral("%1/skin/%2/%3/%4.png").arg(DatapackClientLoader::datapackLoader.getDatapackPath()).arg(entryList.at(entryListIndex)).arg(skinName).arg(type));
            if(pngFile.exists())
                return pngFile.absoluteFilePath();
        }
        {
            QFileInfo gifFile(QStringLiteral("%1/skin/%2/%3/%4.gif").arg(DatapackClientLoader::datapackLoader.getDatapackPath()).arg(entryList.at(entryListIndex)).arg(skinName).arg(type));
            if(gifFile.exists())
                return gifFile.absoluteFilePath();
        }
        entryListIndex++;
    }
    return QString();
}

void SocialChat::on_note_textChanged()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    {
        QSqlQuery query;
        query.prepare("DELETE FROM note WHERE player = (:player)");
        query.bindValue(":player", pseudo);
        if(!query.exec())
            qDebug() << "on_note_textChanged del error:  " << query.lastError();
    }
    {
        QSqlQuery query;
        query.prepare("INSERT INTO note (player,text) VALUES (:player,:text)");
        query.bindValue(":player", pseudo);
        query.bindValue(":text", ui->note->toPlainText());
        if(!query.exec())
            qDebug() << "on_note_textChanged add error:  " << query.lastError();
    }
}

void SocialChat::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &player_type)
{
    if(pseudoToBot.contains(pseudo))
        return;

    ChatEntry newEntry;
    newEntry.player_type=player_type;
    newEntry.player_pseudo=pseudo.toStdString();
    newEntry.type=chat_type;
    newEntry.text=text.toStdString();

    const std::string tempContent=newEntry.player_pseudo+newEntry.text;
    {
        unsigned int index=0;
        while(index<lastText.size())
        {
            const std::unordered_set<std::string> &searchBlock=lastText.at(index);
            if(searchBlock.find(tempContent)!=searchBlock.cend())
                return;
            index++;
        }
    }
    lastText.back().insert(tempContent);

    chat_list << newEntry;
    while(chat_list.size()>64)
        chat_list.removeFirst();
    update_chat();
}

void SocialChat::new_system_text(const CatchChallenger::Chat_type &chat_type,const QString &text)
{
    ChatEntry newEntry;
    newEntry.player_type=CatchChallenger::Player_type_normal;
    //newEntry.player_pseudo=QString();
    newEntry.type=chat_type;
    newEntry.text=text.toStdString();

    const std::string tempContent=newEntry.text;
    {
        unsigned int index=0;
        while(index<lastText.size())
        {
            const std::unordered_set<std::string> &searchBlock=lastText.at(index);
            if(searchBlock.find(tempContent)!=searchBlock.cend())
                return;
            index++;
        }
    }
    lastText.back().insert(tempContent);

    chat_list << newEntry;
    while(chat_list.size()>64)
        chat_list.removeFirst();
    update_chat();
}

void SocialChat::update_chat()
{
    QString nameHtml;
    int index=0;
    while(index<chat_list.size())
    {
        const ChatEntry &entry=chat_list.at(index);
        bool addPlayerInfo=true;
        if(entry.type==CatchChallenger::Chat_type_system || entry.type==CatchChallenger::Chat_type_system_important)
            addPlayerInfo=false;
        if(!addPlayerInfo)
            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(std::string(),CatchChallenger::Player_type_normal,entry.type,entry.text));
        else
            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(entry.player_pseudo,entry.player_type,entry.type,entry.text));
        index++;
    }
    ui->globalChat->setHtml(nameHtml);
    ui->globalChat->verticalScrollBar()->setValue(ui->globalChat->verticalScrollBar()->maximum());
}

void SocialChat::removeNumberForFlood()
{
    if(numberForFlood<=0)
        return;
    numberForFlood--;

    lastText.push_back(std::unordered_set<std::string>());
    if(lastText.size()>6)//6*1.5s=9s
        lastText.erase(lastText.cbegin()+lastText.size()-1);
}

void SocialChat::lineEdit_globalChatText_returnPressed()
{
    QString text=ui->lineEdit_chat_text->text();
    text.remove("\n");
    text.remove("\r");
    text.remove("\t");
    if(text.isEmpty())
        return;
    if(text.contains(QRegularExpression("^ +$")))
    {
        ui->lineEdit_chat_text->clear();
        new_system_text(Chat_type_system,"Space text not allowed");
        return;
    }
    if(text.size()>256)
    {
        ui->lineEdit_chat_text->clear();
        new_system_text(Chat_type_system,"Message too long");
        return;
    }
    if(!text.startsWith('/'))
    {
        if(text==lastMessageSend)
        {
            ui->lineEdit_chat_text->clear();
            new_system_text(Chat_type_system,"Send message like as previous");
            return;
        }
        if(numberForFlood>2)
        {
            ui->lineEdit_chat_text->clear();
            new_system_text(Chat_type_system,"Stop flood");
            return;
        }
    }
    numberForFlood++;
    lastMessageSend=text;
    ui->lineEdit_chat_text->setText(QString());
    if(!text.startsWith("/pm "))
    {
        Chat_type chat_type;
        switch(ui->comboBox_chat_type->itemData(ui->comboBox_chat_type->currentIndex(),99).toUInt())
        {
        default:
        case 0:
            chat_type=Chat_type_all;
        break;
        case 1:
            chat_type=Chat_type_local;
        break;
        case 2:
            chat_type=Chat_type_clan;
        break;
        }
        client->sendChatText(chat_type,text);
        if(!text.startsWith('/'))
            new_chat_text(chat_type,text,QString::fromStdString(client->player_informations.public_informations.pseudo),client->player_informations.public_informations.type);
    }
    else if(text.contains(QRegularExpression("^/pm [^ ]+ .+$")))
    {
        QString pseudo=text;
        pseudo.replace(QRegularExpression("^/pm ([^ ]+) .+$"), "\\1");
        text.replace(QRegularExpression("^/pm [^ ]+ (.+)$"), "\\1");
        client->sendPM(text,pseudo);
        new_chat_text(Chat_type_pm,text,tr("To: ")+pseudo,Player_type_normal);
    }
}
