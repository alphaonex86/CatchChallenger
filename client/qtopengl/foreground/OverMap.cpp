#include "OverMap.hpp"
#include "../Language.hpp"
#include "../../qt/FacilityLibClient.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/QtDatapackClientLoader.hpp"
#include "../ConnexionManager.hpp"
#include "../ChatParsing.hpp"
#include "../CustomText.hpp"
#include "../LineEdit.hpp"
#include "../ComboBox.hpp"
#include "widgets/MapMonsterPreview.hpp"
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <iostream>

OverMap::OverMap()
{
    connexionManager=nullptr;

    playerUI=new QGraphicsPixmapItem(*GameLoader::gameLoader->getImage(":/CC/images/interface/playerui.png"),this);
    playerUI->setVisible(false);
    player=new QGraphicsPixmapItem(playerUI);
    name=new QGraphicsTextItem(playerUI);
    cash=new QGraphicsTextItem(playerUI);

    playersCountBack=new QGraphicsPixmapItem(*GameLoader::gameLoader->getImage(":/CC/images/interface/multicount.png"),this);
    playersCount=new QGraphicsTextItem(this);

    chatBack=new ImagesStrechMiddle(8,":/CC/images/interface/chatBackground.png",this);
    chatBack->setSize(300,400);
    chatText=new QGraphicsTextItem(this);
    chatInput=new LineEdit(this);
    chatInput->setFixedWidth(200);
    chatType=new ComboBox(this);
    chatType->setFixedWidth(100);
    chat=new CustomButton(":/CC/images/interface/chat.png",this);
    {
        QLinearGradient gradient1( 0, 0, 0, 100 );
        gradient1.setColorAt( 0, QColor(255,255,255));
        gradient1.setColorAt( 1, QColor(255,255,255));
        QLinearGradient gradient2( 0, 0, 0, 100 );
        gradient2.setColorAt( 0, QColor(80,71,38));
        gradient2.setColorAt( 1, QColor(80,71,38));
        chatOver=new CustomText(gradient1,gradient2,this);
    }
    chat->setCheckable(true);

    bag=new CustomButton(":/CC/images/interface/bag.png",this);
    {
        QLinearGradient gradient1( 0, 0, 0, 100 );
        gradient1.setColorAt( 0, QColor(255,255,255));
        gradient1.setColorAt( 1, QColor(255,255,255));
        QLinearGradient gradient2( 0, 0, 0, 100 );
        gradient2.setColorAt( 0, QColor(80,71,38));
        gradient2.setColorAt( 1, QColor(80,71,38));
        bagOver=new CustomText(gradient1,gradient2,this);
    }
    buy=new CustomButton(":/CC/images/interface/buy.png",this);
    {
        QLinearGradient gradient1( 0, 0, 0, 100 );
        gradient1.setColorAt( 0, QColor(0,0,0));
        gradient1.setColorAt( 1, QColor(0,0,0));
        QLinearGradient gradient2( 0, 0, 0, 100 );
        gradient2.setColorAt( 0, QColor(255,255,255));
        gradient2.setColorAt( 0.5, QColor(178,242,241));
        buyOver=new CustomText(gradient1,gradient2,this);
    }


/*    if(!connect(back,&CustomButton::clicked,this,&OverMap::backMulti))
        abort();
    if(!connect(server_select,&CustomButton::clicked,this,&OverMap::server_select_clicked))
        abort();
    if(!connect(serverList,&QTreeWidget::itemSelectionChanged,this,&OverMap::itemSelectionChanged))
        abort();*/
    newLanguage();

    if(!connect(&stopFlood,&QTimer::timeout,this,&OverMap::removeNumberForFlood,Qt::QueuedConnection))
        abort();
    if(!connect(chatType,static_cast<void(ComboBox::*)(int)>(&ComboBox::currentIndexChanged),this,&OverMap::comboBox_chat_type_currentIndexChanged))
        abort();
    if(!connect(chatInput,&LineEdit::returnPressed,this,&OverMap::lineEdit_chat_text_returnPressed))
        abort();

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
}

OverMap::~OverMap()
{
}

void OverMap::resetAll()
{
    foreach (MapMonsterPreview *m, monsters)
        delete m;
    monsters.clear();
}

void OverMap::setVar(ConnexionManager *connexionManager)
{
    lastMessageSend.clear();
    this->connexionManager=connexionManager;
    connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::Qtnumber_of_player,this,&OverMap::updatePlayerNumber,Qt::UniqueConnection);
    if(!connexionManager->client->getCaracterSelected())
        connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::Qthave_current_player_info,this,&OverMap::have_current_player_info,Qt::UniqueConnection);
    else
    {
        const CatchChallenger::Player_private_and_public_informations &info=connexionManager->client->get_player_informations_ro();
        have_current_player_info(info);
    }
    chatType->clear();
    chatType->addItem(tr("All"));
    chatType->setItemData(0,0,99);
    chatType->addItem(tr("Local"));
    chatType->setItemData(1,1,99);
    /*if(haveClan)
    {
        chatType->addItem(tr("Clan"));
        chatType->setItemData(2,2,99);
    }*/
    numberForFlood=0;
    connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::Qtnew_chat_text,this,&OverMap::new_chat_text,Qt::UniqueConnection);
    connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::Qtnew_system_text,this,&OverMap::new_system_text,Qt::UniqueConnection);
    updateChat();
}

void OverMap::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    playerUI->setVisible(true);
    QPixmap pPixm=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getFrontSkinPath(informations.public_informations.skinId));
    pPixm=pPixm.scaledToHeight(pPixm.height()*2,Qt::FastTransformation);
    player->setPixmap(pPixm);
    name->setHtml("<span style=\"color:#fff;\">"+QString::fromStdString(informations.public_informations.pseudo)+"<span>");
    std::cout << "info.public_informations.pseudo: " << informations.public_informations.pseudo << std::endl;
    cash->setHtml("<span style=\"color:#fff;\">"+QString::number(informations.cash)+"<span>");
    foreach (MapMonsterPreview *m, monsters)
        delete m;
    monsters.clear();
    for(CatchChallenger::PlayerMonster m : informations.playerMonster)
        monsters.push_back(new MapMonsterPreview(m,this));
    std::pair<uint16_t,uint16_t> t=connexionManager->client->getLast_number_of_player();
    playersCount->setHtml("<span stlye=\"color:#fff;\">"+QString::number(t.first)+"<span>");
}

void OverMap::updatePlayerNumber(const uint16_t &number,const uint16_t &max_players)
{
    Q_UNUSED(max_players);
    playersCount->setHtml("<span style=\"color:#fff;\">"+QString::number(number)+"<span>");
}

void OverMap::newLanguage()
{
    //name->setHtml(tr(""));
    chatOver->setText(tr("Chat"));
    bagOver->setText(tr("Bag"));
    buyOver->setText(tr("Buy"));
    chatType->setItemText(0,tr("All"));
    chatType->setItemText(1,tr("Local"));
    if(chatType->count()>2)
        chatType->setItemText(2,tr("Clan"));
}

QRectF OverMap::boundingRect() const
{
    return QRectF();
}

void OverMap::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
{
    unsigned int space=10;
    if(playerUI->isVisible())
    {
        playerUI->setPos(space,space);
        player->setPos(0,-10);
        name->setPos(playerUI->x()+160,playerUI->y()+10);
        cash->setPos(playerUI->x()+220,playerUI->y()+57);
        unsigned int monsterIndex=0;
        while(monsterIndex<monsters.size())
        {
            MapMonsterPreview *m=monsters.at(monsterIndex);
            m->setPos(playerUI->x()+425+space+monsterIndex*(space+56),playerUI->y());
            monsterIndex++;
        }
    }

    playersCountBack->setPos(w->width()-space-playersCountBack->pixmap().width(),space);
    playersCount->setPos(w->width()-space-80,space+15);

    int logicalDpiX=w->logicalDpiX();
    int logicalDpiY=w->logicalDpiY();
    int physicalDpiX=w->physicalDpiX();
    int physicalDpiY=w->physicalDpiY();

    chat->setSize(84,93);
    unsigned int chatX=space;
    unsigned int chatY=w->height()-space-chat->height();
    chat->setPos(chatX,chatY);
    chatOver->setPixelSize(18);
    chatOver->setPos(chatX+chat->width()/2-chatOver->boundingRect().width()/2,w->height()-space-chatOver->boundingRect().height());
    chatBack->setVisible(chat->isChecked());
    chatText->setVisible(chat->isChecked());
    chatInput->setVisible(chat->isChecked());
    chatType->setVisible(chat->isChecked());
    if(chat->isChecked())
    {
        unsigned int y=chatY-3-chatInput->height();
        chatInput->setPos(space,y);
        chatType->setPos(space+chatInput->width()+2,y);

        y-=3+chatBack->height();
        chatBack->setPos(space,y);
        chatText->setVisible(physicalDpiX<200);
        chatText->setPos(space+space,y+space);
    }

    //compose from right to left
    unsigned int x=w->width()-space;
    if(buy->isVisible())
    {
        buy->setSize(84,93);
        unsigned int buyX=x-buy->width();
        buy->setPos(buyX,w->height()-space-buy->height());
        x-=buy->width()+space;
        buyOver->setVisible(physicalDpiX<200);
        buyOver->setPixelSize(18);
        buyOver->setPos(buyX+buy->width()/2-buyOver->boundingRect().width()/2,w->height()-space-buyOver->boundingRect().height());
    }
    {
        bag->setSize(84,93);
        unsigned int bagX=x-bag->width();
        bag->setPos(bagX,w->height()-space-bag->height());
        x-=bag->width()+space;
        bagOver->setVisible(physicalDpiX<200);
        bagOver->setPixelSize(18);
        bagOver->setPos(bagX+bag->width()/2-bagOver->boundingRect().width()/2,w->height()-space-bagOver->boundingRect().height());
    }
    Q_UNUSED(x);
}

void OverMap::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    chat->mousePressEventXY(p,pressValidated);
    buy->mousePressEventXY(p,pressValidated);
    bag->mousePressEventXY(p,pressValidated);
}

void OverMap::mouseReleaseEventXY(const QPointF &p, bool &pressValidated)
{
    chat->mouseReleaseEventXY(p,pressValidated);
    buy->mouseReleaseEventXY(p,pressValidated);
    bag->mouseReleaseEventXY(p,pressValidated);
}

void OverMap::lineEdit_chat_text_returnPressed()
{
    QString text=chatInput->text();
    text.remove("\n");
    text.remove("\r");
    text.remove("\t");
    if(text.isEmpty())
        return;
    if(text.contains(QRegularExpression("^ +$")))
    {
        chatInput->clear();
        connexionManager->client->add_system_text(CatchChallenger::Chat_type_system,"Space text not allowed");
        return;
    }
    if(text.size()>256)
    {
        chatInput->clear();
        connexionManager->client->add_system_text(CatchChallenger::Chat_type_system,"Message too long");
        return;
    }
    if(!text.startsWith('/'))
    {
        if(text.toStdString()==lastMessageSend)
        {
            chatInput->clear();
            connexionManager->client->add_system_text(CatchChallenger::Chat_type_system,"Send message like as previous");
            return;
        }
        if(numberForFlood>2)
        {
            chatInput->clear();
            connexionManager->client->add_system_text(CatchChallenger::Chat_type_system,"Stop flood");
            return;
        }
    }
    numberForFlood++;
    lastMessageSend=text.toStdString();
    chatInput->setText(QString());
    if(!text.startsWith("/pm "))
    {
        CatchChallenger::Chat_type chat_type;
        switch(chatType->itemData(chatType->currentIndex(),99).toUInt())
        {
        default:
        case 0:
            chat_type=CatchChallenger::Chat_type_all;
        break;
        case 1:
            chat_type=CatchChallenger::Chat_type_local;
        break;
        case 2:
            chat_type=CatchChallenger::Chat_type_clan;
        break;
        }
        connexionManager->client->sendChatText(chat_type,text.toStdString());
        if(!text.startsWith('/'))
            connexionManager->client->add_chat_text(chat_type,text.toStdString(),connexionManager->client->player_informations.public_informations.pseudo,
                 connexionManager->client->player_informations.public_informations.type);
    }
    else if(text.contains(QRegularExpression("^/pm [^ ]+ .+$")))
    {
        QString pseudo=text;
        pseudo.replace(QRegularExpression("^/pm ([^ ]+) .+$"), "\\1");
        text.replace(QRegularExpression("^/pm [^ ]+ (.+)$"), "\\1");
        connexionManager->client->sendPM(text.toStdString(),pseudo.toStdString());
        connexionManager->client->add_chat_text(CatchChallenger::Chat_type_pm,text.toStdString(),tr("To: ").toStdString()+pseudo.toStdString(),CatchChallenger::Player_type_normal);
    }
    updateChat();
}

void OverMap::comboBox_chat_type_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    updateChat();
}

void OverMap::new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text)
{
    updateChat();
}

void OverMap::new_chat_text(CatchChallenger::Chat_type chat_type,std::string text,std::string pseudo,CatchChallenger::Player_type player_type)
{
    updateChat();
}

void OverMap::updateChat()
{
    const std::vector<CatchChallenger::Api_protocol::ChatEntry> &chat_list=connexionManager->client->getChatContent();
    QString nameHtml;
    unsigned int index=0;
    while(index<chat_list.size())
    {
        const CatchChallenger::Api_protocol::ChatEntry &entry=chat_list.at(index);
        bool addPlayerInfo=true;
        if(entry.chat_type==CatchChallenger::Chat_type_system || entry.chat_type==CatchChallenger::Chat_type_system_important)
            addPlayerInfo=false;
        if(!addPlayerInfo)
            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(std::string(),CatchChallenger::Player_type_normal,entry.chat_type,entry.text));
        else
            nameHtml+=QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(entry.player_pseudo,entry.player_type,entry.chat_type,entry.text));
        index++;
    }
    chatText->setHtml(nameHtml);
}

void OverMap::removeNumberForFlood()
{
    if(numberForFlood<=0)
        return;
    numberForFlood--;
}
