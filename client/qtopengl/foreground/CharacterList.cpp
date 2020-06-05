#include "CharacterList.hpp"
#include "../Language.hpp"
#include "../ClientStructures.hpp"
#include "../ConnexionManager.hpp"
#include "../above/AddCharacter.hpp"
#include "../above/NewGame.hpp"
#include "../../qt/FacilityLibClient.hpp"
#include "../../qt/QtDatapackClientLoader.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonDatapack.hpp"

CharacterList::CharacterList()
{
    addCharacter=nullptr;
    newGame=nullptr;
    serverSelected=0;
    profileIndex=0;

    add=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    add->setOutlineColor(QColor(44,117,0));
    add->setEnabled(false);
    remove=new CustomButton(":/CC/images/interface/delete.png",this);
    remove->setOutlineColor(QColor(125,0,0));
    remove->setEnabled(false);

    characterEntryListProxy=new QGraphicsProxyWidget(this);
    characterEntryList=new QListWidget();
    characterEntryList->setIconSize(QSize(80, 80));
    characterEntryList->setUniformItemSizes(true);
    characterEntryListProxy->setWidget(characterEntryList);
    characterEntryListProxy->setZValue(1);

    select=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);
    select->setEnabled(false);

    wdialog=new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this);
    warning=new QGraphicsTextItem(this);
    warning->setVisible(false);

    if(!connect(add,&CustomButton::clicked,this,&CharacterList::add_clicked))
        abort();
    if(!connect(remove,&CustomButton::clicked,this,&CharacterList::remove_clicked))
        abort();
    if(!connect(select,&CustomButton::clicked,this,&CharacterList::select_clicked))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&CharacterList::backSubServer))
        abort();
    if(!connect(characterEntryList,&QListWidget::itemSelectionChanged,this,&CharacterList::itemSelectionChanged))
        abort();
    newLanguage();

    //need be the last
    addCharacter=nullptr;
}

CharacterList::~CharacterList()
{
}

void CharacterList::itemSelectionChanged()
{
    const QList<QListWidgetItem *> &selectedItems=characterEntryList->selectedItems();
    select->setEnabled(selectedItems.size()==1);
    remove->setEnabled(selectedItems.size()==1 && characterEntryList->count()>CommonSettingsCommon::commonSettingsCommon.min_character);
}

void CharacterList::add_clicked()
{
    profileIndex=0;
    const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList=connexionManager->client->getServerOrdenedList();
    if((characterEntryListInWaiting.size()+characterListForSelection.at(serverOrdenedList.at(serverSelected).charactersGroupIndex).size())>=
            CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        warning->setHtml(tr("You have already the max characters count"));
        warning->setVisible(true);
        return;
    }
    if(addCharacter==nullptr)
    {
        addCharacter=new AddCharacter();
        if(!connect(addCharacter,&AddCharacter::removeAbove,this,&CharacterList::add_finished))
            abort();
    }
    addCharacter->setDatapack(connexionManager->client->datapackPathBase());
    emit setAbove(addCharacter);
}

void CharacterList::add_finished()
{
    if(!addCharacter->isOk())
    {
        emit setAbove(nullptr);
        if(characterEntryList->count()<CommonSettingsCommon::commonSettingsCommon.min_character)
            emit backSubServer();
        return;
    }
    const std::string &datapackPath=connexionManager->client->datapackPathBase();
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.size()>1)
        profileIndex=addCharacter->getProfileIndex();
    if(profileIndex>=CatchChallenger::CommonDatapack::commonDatapack.profileList.size())
        return;
    CatchChallenger::Profile profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(newGame==nullptr)
    {
        newGame=new NewGame();
        if(!connect(newGame,&NewGame::removeAbove,this,&CharacterList::newGame_finished))
            abort();
    }
    newGame->setDatapack(datapackPath+DATAPACK_BASE_PATH_SKIN,datapackPath+DATAPACK_BASE_PATH_MONSTERS,profile.monstergroup,profile.forcedskin);
    if(!newGame->haveSkin())
    {
        warning->setHtml(tr("Sorry but no skin found into: %1").arg(QFileInfo(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_SKIN).absoluteFilePath()));
        warning->setVisible(true);
        return;
    }
    emit setAbove(newGame);
}

void CharacterList::newGame_finished()
{
    if(!newGame->isOk())
    {
        emit setAbove(nullptr);
        if(characterEntryList->count()<CommonSettingsCommon::commonSettingsCommon.min_character)
            emit backSubServer();
        return;
    }
    const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList=connexionManager->client->getServerOrdenedList();
    CatchChallenger::CharacterEntry characterEntry;
    characterEntry.character_id=0;
    characterEntry.delete_time_left=0;
    characterEntry.last_connect=QDateTime::currentMSecsSinceEpoch()/1000;
    //characterEntry.mapId=QtDatapackClientLoader::datapackLoader->mapToId.value(profile.map);
    characterEntry.played_time=0;
    characterEntry.pseudo=newGame->pseudo();
    characterEntry.charactersGroupIndex=newGame->monsterGroupId();
    characterEntry.skinId=newGame->skinId();
    connexionManager->client->addCharacter(serverOrdenedList.at(serverSelected).charactersGroupIndex,
                         static_cast<uint8_t>(profileIndex),characterEntry.pseudo,
                         characterEntry.charactersGroupIndex,characterEntry.skinId);
    characterEntryListInWaiting.push_back(characterEntry);
    if((characterEntryListInWaiting.size()+characterListForSelection.at(serverOrdenedList.at(serverSelected).charactersGroupIndex).size())
            >=CommonSettingsCommon::commonSettingsCommon.max_character)
        add->setEnabled(false);
    warning->setHtml(tr("Creating your new character"));
    warning->setVisible(true);
}

void CharacterList::remove_clicked()
{
    const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList=connexionManager->client->getServerOrdenedList();
    QList<QListWidgetItem *> selectedItems=characterEntryList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const uint32_t &character_id=selectedItems.first()->data(99).toUInt();
    const uint32_t &delete_time_left=selectedItems.first()->data(98).toUInt();
    if(delete_time_left>0)
    {
        warning->setHtml(tr("Deleting already planned"));
        warning->setVisible(true);
        return;
    }
    connexionManager->client->removeCharacter(serverOrdenedList.at(serverSelected).charactersGroupIndex,character_id);
    unsigned int index=0;
    while(index<characterListForSelection.at(serverOrdenedList.at(serverSelected).charactersGroupIndex).size())
    {
        const CatchChallenger::CharacterEntry &characterEntry=characterListForSelection.at(serverOrdenedList.at(serverSelected).charactersGroupIndex).at(index);
        if(characterEntry.character_id==character_id)
        {
            characterListForSelection[serverOrdenedList.at(serverSelected).charactersGroupIndex][index].delete_time_left=
                    CommonSettingsCommon::commonSettingsCommon.character_delete_time;
            break;
        }
        index++;
    }
    updateCharacterList();
    warning->setHtml(tr("Your charater will be deleted into %1").arg(QString::fromStdString(
                                      CatchChallenger::FacilityLibClient::timeToString(CommonSettingsCommon::commonSettingsCommon.character_delete_time))
                                  )
                             );
    warning->setVisible(true);
}

void CharacterList::select_clicked()
{
    QList<QListWidgetItem *> selectedItems=characterEntryList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    emit selectCharacter(serverSelected,selectedItems.first()->data(99).toUInt());
    //emit toLoading(tr("Selecting your character"));
}

void CharacterList::newLanguage()
{
    add->setText(tr("Add"));
    //remove->setText(tr("Remove"));
    warning->setHtml("<span style=\"background-color: rgb(255, 255, 163);\nborder: 1px solid rgb(255, 221, 50);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\">"+tr("Loading the servers list...")+"</span>");
}

QRectF CharacterList::boundingRect() const
{
    return QRectF();
}

void CharacterList::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
{
    unsigned int space=10;
    unsigned int fontSize=20;
    unsigned int multiItemH=100;
    if(w->height()>300)
        fontSize=w->height()/6;
    if(w->width()<600 || w->height()<600)
    {
        add->setSize(148,61);
        add->setPixelSize(23);
        remove->setSize(56,62);
        select->setSize(56,62);
        back->setSize(56,62);
        multiItemH=50;
    }
    else if(w->width()<900 || w->height()<600)
    {
        add->setSize(148,61);
        add->setPixelSize(23);
        remove->setSize(84,93);
        select->setSize(84,93);
        back->setSize(84,93);
        multiItemH=75;
    }
    else {
        space=30;
        add->setSize(223,92);
        add->setPixelSize(35);
        remove->setSize(84,93);
        select->setSize(84,93);
        back->setSize(84,93);
    }


    unsigned int tWidthTButton=add->width()+space+remove->width();
    unsigned int tXTButton=w->width()/2-tWidthTButton/2;
    unsigned int tWidthTButtonOffset=0;
    unsigned int y=w->height()-space-select->height()-space-add->height();
    remove->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=remove->width()+space;
    add->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=add->width()+space;

    tWidthTButton=select->width()+space+
            back->width();
    y=w->height()-space-select->height();
    tXTButton=w->width()/2-tWidthTButton/2;
    back->setPos(tXTButton,y);
    select->setPos(tXTButton+back->width()+space,y);

    y=w->height()-space-select->height()-space-add->height();
    wdialog->setHeight(w->height()-space-select->height()-space-add->height()-space-space);
    if((unsigned int)w->width()<(800+space*2))
    {
        wdialog->setWidth(w->width()-space*2);
        wdialog->setPos(space,space);
    }
    else
    {
        wdialog->setWidth(800);
        wdialog->setPos(w->width()/2-wdialog->width()/2,space);
    }
    warning->setPos(w->width()/2-warning->boundingRect().width(),space+wdialog->height()-wdialog->currentBorderSize()-warning->boundingRect().height());
    characterEntryListProxy->setPos(wdialog->x()+space,wdialog->y()+space);
    characterEntryList->setFixedSize(wdialog->width()-space-space,wdialog->height()-space-space);
}

void CharacterList::mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    add->mousePressEventXY(p,pressValidated);
    remove->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    select->mousePressEventXY(p,pressValidated);
}

void CharacterList::mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    add->mouseReleaseEventXY(p,pressValidated);
    remove->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    select->mouseReleaseEventXY(p,pressValidated);
}

void CharacterList::connectToSubServer(const int indexSubServer,ConnexionManager *connexionManager,const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    if(!connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QtnewCharacterId,this,&CharacterList::newCharacterId))
        abort();
    //detect character group to display only characters in this group
    this->serverSelected=indexSubServer;
    this->connexionManager=connexionManager;
    this->characterListForSelection=characterEntryList;
    updateCharacterList();
}

void CharacterList::updateCharacterList()
{
    const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList=connexionManager->client->getServerOrdenedList();//do a crash due to reference
    //const std::vector<ServerFromPoolForDisplay> serverOrdenedList=connexionManager->client->getServerOrdenedList();
    //do the grouping for characterGroup count
    {
        serverByCharacterGroup.clear();
        unsigned int index=0;
        uint8_t serverByCharacterGroupTempIndexToDisplay=1;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=serverOrdenedList.at(index);
            if(server.charactersGroupIndex>serverOrdenedList.size())
                abort();
            if(serverByCharacterGroup.find(server.charactersGroupIndex)!=serverByCharacterGroup.cend())
                serverByCharacterGroup[server.charactersGroupIndex].first++;
            else
            {
                serverByCharacterGroup[server.charactersGroupIndex].first=1;
                serverByCharacterGroup[server.charactersGroupIndex].second=serverByCharacterGroupTempIndexToDisplay;
                serverByCharacterGroupTempIndexToDisplay++;
            }
            index++;
        }
    }
    characterEntryList->clear();
    unsigned int index=0;
    const uint8_t charactersGroupIndex=serverOrdenedList.at(serverSelected).charactersGroupIndex;
    while(index<characterListForSelection.at(charactersGroupIndex).size())
    {
        const CatchChallenger::CharacterEntry &characterEntry=characterListForSelection.at(charactersGroupIndex).at(index);
        QListWidgetItem * item=new QListWidgetItem();
        item->setData(99,characterEntry.character_id);
        item->setData(98,characterEntry.delete_time_left);
        //item->setData(97,characterEntry.mapId);
        std::string text=characterEntry.pseudo+"\n";
        if(characterEntry.played_time>0)
            text+=tr("%1 played")
                    .arg(QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(characterEntry.played_time)))
                    .toStdString();
        else
            text+=tr("Never played").toStdString();
        if(characterEntry.delete_time_left>0)
            text+="\n"+tr("%1 to be deleted")
                    .arg(QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(characterEntry.delete_time_left)))
                    .toStdString();
        /*if(characterEntry.mapId==-1)
            text+="\n"+tr("Map missing, can't play");*/
        item->setText(QString::fromStdString(text));
        if(characterEntry.skinId<QtDatapackClientLoader::datapackLoader->skins.size())
            item->setIcon(QIcon(
                              QString::fromStdString(connexionManager->client->datapackPathBase())+
                              DATAPACK_BASE_PATH_SKIN+
                              QString::fromStdString(QtDatapackClientLoader::datapackLoader->skins.at(characterEntry.skinId))+
                              "/front.png"
                              ));
        else
            item->setIcon(QIcon(QStringLiteral(":/CC/images/player_default/front.png")));
        characterEntryList->addItem(item);
        index++;
    }
    int charaterCount=characterListForSelection.at(charactersGroupIndex).size();
    add->setEnabled(charaterCount<CommonSettingsCommon::commonSettingsCommon.max_character);
    if(charaterCount<CommonSettingsCommon::commonSettingsCommon.min_character && charaterCount<CommonSettingsCommon::commonSettingsCommon.max_character)
        add_clicked();
    else if(charaterCount==CommonSettingsCommon::commonSettingsCommon.min_character && charaterCount==CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        characterEntryList->itemAt(0,0)->setSelected(true);
        select_clicked();
    }
}

void CharacterList::newCharacterId(const uint8_t &returnCode, const uint32_t &characterId)
{
    const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList=connexionManager->client->getServerOrdenedList();
    CatchChallenger::CharacterEntry characterEntry=characterEntryListInWaiting.front();
    characterEntryListInWaiting.erase(characterEntryListInWaiting.cbegin());
    if(returnCode==0x00)
    {
        characterEntry.character_id=characterId;
        const uint8_t charactersGroupIndex=serverOrdenedList.at(serverSelected).charactersGroupIndex;
        characterListForSelection[charactersGroupIndex].push_back(characterEntry);
        updateCharacterList();
        characterEntryList->item(characterEntryList->count()-1)->setSelected(true);
        if(characterEntryList->count()>=CommonSettingsCommon::commonSettingsCommon.min_character &&
                characterEntryList->count()<=CommonSettingsCommon::commonSettingsCommon.max_character)
            select_clicked();
    }
    else
    {
        if(returnCode==0x01)
            warning->setHtml(tr("This pseudo is already taken"));
        else if(returnCode==0x02)
            warning->setHtml(tr("Have already the max caraters taken"));
        else
            warning->setHtml(tr("Unable to create the character"));
        warning->setVisible(true);
    }
}
