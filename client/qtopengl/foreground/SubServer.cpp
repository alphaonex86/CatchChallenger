#include "SubServer.hpp"
#include "../Language.hpp"
#include "../../qt/FacilityLibClient.hpp"
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <iostream>

SubServer::SubServer()
{
    serverListProxy=new QGraphicsProxyWidget(this);
    serverList=new QTreeWidget();
    serverList->setAlternatingRowColors(true);
    serverList->setIconSize(QSize(16, 16));
    serverList->setRootIsDecorated(false);
    serverList->header()->setSectionResizeMode(QHeaderView::Fixed);
    serverList->header()->resizeSection(0,580);
    serverListProxy->setWidget(serverList);
    serverListProxy->setZValue(1);

    server_select=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);
    server_select->setEnabled(false);

    wdialog=new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this);
    connexionManager=nullptr;
    averagePlayedTime=0;
    averageLastConnect=0;

    if(!connect(back,&CustomButton::clicked,this,&SubServer::backMulti))
        abort();
    if(!connect(server_select,&CustomButton::clicked,this,&SubServer::server_select_clicked))
        abort();
    if(!connect(serverList,&QTreeWidget::itemSelectionChanged,this,&SubServer::itemSelectionChanged))
        abort();
    newLanguage();
}

SubServer::~SubServer()
{
}

void SubServer::server_select_clicked()
{
    const QList<QTreeWidgetItem *> &selectedItems=serverList->selectedItems();
    if(selectedItems.size()!=1)
        return;

    const QTreeWidgetItem * const selectedItem=selectedItems.at(0);
    const int serverSelected=selectedItem->data(99,99).toUInt();
    emit connectToSubServer(serverSelected);
}

void SubServer::newLanguage()
{
    QTreeWidgetItem *___qtreewidgetitem = serverList->headerItem();
    ___qtreewidgetitem->setText(0, tr("Server"));
    ___qtreewidgetitem->setText(1, tr("Players"));
}

QRectF SubServer::boundingRect() const
{
    return QRectF();
}

void SubServer::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
{
    unsigned int space=10;
    unsigned int fontSize=20;
    unsigned int multiItemH=100;
    if(w->height()>300)
        fontSize=w->height()/6;
    if(w->width()<600 || w->height()<600)
    {
        server_select->setSize(56,62);
        back->setSize(56,62);
        multiItemH=50;
    }
    else if(w->width()<900 || w->height()<600)
    {
        server_select->setSize(84,93);
        back->setSize(84,93);
        multiItemH=75;
    }
    else {
        space=30;
        server_select->setSize(84,93);
        back->setSize(84,93);
    }


    unsigned int tWidthTButton=server_select->width()+space+
            back->width();
    unsigned int y=w->height()-space-server_select->height();
    unsigned int tXTButton=w->width()/2-tWidthTButton/2;
    back->setPos(tXTButton,y);
    server_select->setPos(tXTButton+back->width()+space,y);

    y=w->height()-space-server_select->height();
    wdialog->setHeight(w->height()-space-server_select->height()-space-space);
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
    serverListProxy->setPos(wdialog->x()+space,wdialog->y()+space);
    serverList->setFixedSize(wdialog->width()-space-space,wdialog->height()-space-space);
    serverList->header()->resizeSection(0,wdialog->width()-space-space-120);
}

void SubServer::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    back->mousePressEventXY(p,pressValidated);
    server_select->mousePressEventXY(p,pressValidated);
}

void SubServer::mouseReleaseEventXY(const QPointF &p, bool &pressValidated)
{
    back->mouseReleaseEventXY(p,pressValidated);
    server_select->mouseReleaseEventXY(p,pressValidated);
}

void SubServer::itemSelectionChanged()
{
    const QList<QTreeWidgetItem *> &selectedItems=serverList->selectedItems();
    server_select->setEnabled(selectedItems.size()==1);
}

void SubServer::logged(std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList,ConnexionManager *connexionManager)
{
    this->connexionManager=connexionManager;
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

    //clear and determine what kind of view
    serverList->clear();
    CatchChallenger::LogicialGroup logicialGroup=connexionManager->client->getLogicialGroup();
    bool fullView=true;
    if(serverOrdenedList.size()>10)
        fullView=false;
    const uint64_t &current__date=QDateTime::currentDateTime().toTime_t();

    //reload, bug if before init
    if(icon_server_list_star1.isNull())
    {
        SubServer::icon_server_list_star1=QIcon(":/CC/images/interface/server_list/star1.png");
        if(SubServer::icon_server_list_star1.isNull())
            abort();
        SubServer::icon_server_list_star2=QIcon(":/CC/images/interface/server_list/star2.png");
        SubServer::icon_server_list_star3=QIcon(":/CC/images/interface/server_list/star3.png");
        SubServer::icon_server_list_star4=QIcon(":/CC/images/interface/server_list/star4.png");
        SubServer::icon_server_list_star5=QIcon(":/CC/images/interface/server_list/star5.png");
        SubServer::icon_server_list_star6=QIcon(":/CC/images/interface/server_list/star6.png");
        SubServer::icon_server_list_stat1=QIcon(":/CC/images/interface/server_list/stat1.png");
        SubServer::icon_server_list_stat2=QIcon(":/CC/images/interface/server_list/stat2.png");
        SubServer::icon_server_list_stat3=QIcon(":/CC/images/interface/server_list/stat3.png");
        SubServer::icon_server_list_stat4=QIcon(":/CC/images/interface/server_list/stat4.png");
        SubServer::icon_server_list_bug=QIcon(":/CC/images/interface/server_list/bug.png");
        if(SubServer::icon_server_list_bug.isNull())
            abort();
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/0.png"));
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/1.png"));
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/2.png"));
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/3.png"));
    }
    //do the average value
    {
        averagePlayedTime=0;
        averageLastConnect=0;
        int entryCount=0;
        unsigned int index=0;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=serverOrdenedList.at(index);
            if(server.playedTime>0 && server.lastConnect<=current__date)
            {
                averagePlayedTime+=server.playedTime;
                averageLastConnect+=server.lastConnect;
                entryCount++;
            }
            index++;
        }
        if(entryCount>0)
        {
            averagePlayedTime/=entryCount;
            averageLastConnect/=entryCount;
        }
    }
    addToServerList(logicialGroup,serverList->invisibleRootItem(),current__date,fullView);
    serverList->expandAll();
}

bool CatchChallenger::ServerFromPoolForDisplay::operator<(const ServerFromPoolForDisplay &serverFromPoolForDisplay) const
{
    if(serverFromPoolForDisplay.uniqueKey<this->uniqueKey)
        return true;
    else
        return false;
}

void SubServer::addToServerList(CatchChallenger::LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView)
{
    if(connexionManager->client->getServerOrdenedList().empty())
        std::cerr << "SubServer::addToServerList(): client->serverOrdenedList.empty()" << std::endl;
    item->setText(0,QString::fromStdString(logicialGroup.name));
    {
        //to order the group
        std::vector<std::string> keys;
        for(const auto &n : logicialGroup.logicialGroupList)
            keys.push_back(n.first);
        qSort(keys);
        //list the group
        unsigned int index=0;
        while(index<keys.size())
        {
            QTreeWidgetItem * const itemGroup=new QTreeWidgetItem(item);
            addToServerList(*logicialGroup.logicialGroupList[keys.at(index)],itemGroup,currentDate,fullView);
            index++;
        }
    }
    {
        qSort(logicialGroup.servers);
        //list the server
        unsigned int index=0;
        while(index<logicialGroup.servers.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=logicialGroup.servers.at(index);
            QTreeWidgetItem *itemServer=new QTreeWidgetItem(item);
            std::string text;
            std::string groupText;
            if(/*characterListForSelection.size()>1 && */serverByCharacterGroup.size()>1)
            {
                const uint8_t groupInt=serverByCharacterGroup.at(server.charactersGroupIndex).second;
                //comment the if to always show it
                if(groupInt>=icon_server_list_color.size())
                    groupText=QStringLiteral(" (%1)").arg(groupInt).toStdString();
                itemServer->setToolTip(0,tr("Server group: %1, UID: %2").arg(groupInt).arg(server.uniqueKey));
                if(!icon_server_list_color.empty())
                    itemServer->setIcon(0,icon_server_list_color.at(groupInt%icon_server_list_color.size()));
            }
            std::string name=server.name;
            if(name.empty())
                name=tr("Default server").toStdString();
            if(fullView)
            {
                text=name+groupText;
                if(server.playedTime>0)
                {
                    if(!server.description.empty())
                        text+=" "+tr("%1 played").arg(QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(server.playedTime))).toStdString();
                    else
                        text+="\n"+tr("%1 played").arg(QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(server.playedTime))).toStdString();
                }
                if(!server.description.empty())
                    text+="\n"+server.description;
            }
            else
            {
                if(server.description.empty())
                    text=name+groupText;
                else
                    text=name+groupText+" - "+server.description;
            }
            itemServer->setText(0,QString::fromStdString(text));

            //do the icon here
            if(server.playedTime>5*365*24*3600)
            {
                itemServer->setIcon(0,SubServer::icon_server_list_bug);
                itemServer->setToolTip(0,tr("Played time greater than 5y, bug?"));
            }
            else if(server.lastConnect>0 && server.lastConnect<1420070400)
            {
                itemServer->setIcon(0,SubServer::icon_server_list_bug);
                itemServer->setToolTip(0,tr("Played before 2015, bug?"));
            }
            else if(server.maxPlayer<=65533 && (server.maxPlayer<server.currentPlayer || server.maxPlayer==0))
            {
                itemServer->setIcon(0,SubServer::icon_server_list_bug);
                if(server.maxPlayer<server.currentPlayer)
                    itemServer->setToolTip(0,tr("maxPlayer<currentPlayer"));
                else
                    itemServer->setToolTip(0,tr("maxPlayer==0"));
            }
            else if(server.playedTime>0 || server.lastConnect>0)
            {
                uint64_t dateDiff=0;
                if(currentDate>server.lastConnect)
                    dateDiff=currentDate-server.lastConnect;
                if(server.playedTime>24*3600*31)
                {
                    if(dateDiff<24*3600)
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star1);
                        itemServer->setToolTip(0,tr("Played time greater than 24h, last connect in this last 24h"));
                    }
                    else
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star2);
                        itemServer->setToolTip(0,tr("Played time greater than 24h, last connect not in this last 24h"));
                    }
                }
                else if(server.lastConnect<averageLastConnect)
                {
                    if(server.playedTime<averagePlayedTime)
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star3);
                        itemServer->setToolTip(0,tr("Into the more recent server used, out of the most used server"));
                    }
                    else
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star4);
                        itemServer->setToolTip(0,tr("Into the more recent server used, into the most used server"));
                    }
                }
                else
                {
                    if(server.playedTime<averagePlayedTime)
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star5);
                        itemServer->setToolTip(0,tr("Out of the more recent server used, out of the most used server"));
                    }
                    else
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star6);
                        itemServer->setToolTip(0,tr("Out of the more recent server used, into the most used server"));
                    }
                }

            }
            if(server.maxPlayer<=65533)
            {
                //do server.currentPlayer/server.maxPlayer icon
                if(server.maxPlayer<=0 || server.currentPlayer>server.maxPlayer)
                    itemServer->setIcon(1,SubServer::icon_server_list_bug);
                else
                {
                    //to be very sure
                    if(server.maxPlayer>0)
                    {
                        int percent=(server.currentPlayer*100)/server.maxPlayer;
                        if(server.currentPlayer==server.maxPlayer || (server.maxPlayer>50 && percent>98))
                            itemServer->setIcon(1,SubServer::icon_server_list_stat4);
                        else if(server.currentPlayer>30 && percent>50)
                            itemServer->setIcon(1,SubServer::icon_server_list_stat3);
                        else if(server.currentPlayer>5 && percent>20)
                            itemServer->setIcon(1,SubServer::icon_server_list_stat2);
                        else
                            itemServer->setIcon(1,SubServer::icon_server_list_stat1);
                    }
                }
                itemServer->setText(1,QStringLiteral("%1/%2").arg(server.currentPlayer).arg(server.maxPlayer));
            }
            const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList=connexionManager->client->getServerOrdenedList();
            if(server.serverOrdenedListIndex<serverOrdenedList.size())
                itemServer->setData(99,99,server.serverOrdenedListIndex);
            else
            {
                std::cerr << "SubServer::addToServerList(): server.serverOrdenedListIndex>=serverOrdenedList.size(), "+
                      std::to_string(server.serverOrdenedListIndex)+
                      ">="+
                      std::to_string(serverOrdenedList.size())+
                      ", error";
                return;
            }
            /*if(serverList->iconSize()>100)
            {
                itemServer->setIcon(0,SubServer::icon_server_list_stat3);
            }
            else
                itemServer->setIcon(0,QIcon());*/
            index++;
        }
    }
}

