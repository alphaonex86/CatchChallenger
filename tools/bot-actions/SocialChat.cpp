#include "SocialChat.h"
#include "ui_SocialChat.h"
#include "../../client/base/interface/DatapackClientLoader.h"

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
}

void SocialChat::showEvent(QShowEvent * event)
{
    QHash<CatchChallenger::Api_protocol *,ActionsBotInterface::Player>::const_iterator i = ActionsBotInterface::clientList.constBegin();
    while (i != ActionsBotInterface::clientList.constEnd()) {
        ActionsBotInterface::Player &client=const_cast<ActionsBotInterface::Player &>(i.value());
        if(client.api->getCaracterSelected())
        {
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
