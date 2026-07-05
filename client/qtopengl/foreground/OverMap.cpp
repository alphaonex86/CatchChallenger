#include "OverMap.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../libqtcatchchallenger/Ultimate.hpp"
#include "../../libqtcatchchallenger/Settings.hpp"
#include "../background/CCMap.hpp"
#include <QKeyEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QInputDevice>
#if defined(CATCHCHALLENGER_SOLO) && ! defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
#include "../../../server/qt/InternalServer.hpp"
#endif
#include "../ConnexionManager.hpp"
#include "../ChatParsing.hpp"
#include "../CustomText.hpp"
#include "../LineEdit.hpp"
#include "../ComboBox.hpp"
#include "../QGraphicsPixmapItemClick.hpp"
#include "../above/MonsterDetails.hpp"
#include "widgets/MapMonsterPreview.hpp"
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <iostream>
#include <QDesktopServices>
#include <QTextDocument>
#include <QTextOption>
#include <QColor>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QAbstractTextDocumentLayout>

//Configure a QGraphicsTextItem used as an overlay text body (pure scene
//rendering — works on Android/OpenGL ES, unlike an embedded QWidget). The wrap
//mode breaks even inside a long unbreakable token, so once setTextWidth() is
//given the text can never spill past that width.
static void configureOverlayText(QGraphicsTextItem *t,const QString &textColor)
{
    t->setDefaultTextColor(QColor(textColor));
    QTextOption opt=t->document()->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    t->document()->setDefaultTextOption(opt);
}

int OverMap::touchControlsCachedValue=-1;

OverMap::OverMap()
{
    connexionManager=nullptr;
    monstersDragged=nullptr;
    wasDragged=false;
    monsterDetails=nullptr;
    ccmap=nullptr;
    touchControlsActive=false;
    IG_dialog_link_selected=-1;

    playerBackground=new QGraphicsPixmapItemClick(QPixmap(":/CC/images/interface/playerui.png"),this);
    playerBackgroundBig=true;
    playerBackground->setVisible(false);
    playerImage=new QGraphicsPixmapItem(playerBackground);
    name=new QGraphicsTextItem(playerBackground);
    cash=new QGraphicsTextItem(playerBackground);

    playersCountBack=new QGraphicsPixmapItem(QPixmap(":/CC/images/interface/multicount.png"),this);
    playersCount=new QGraphicsTextItem(this);

    chatBack=new ImagesStrechMiddle(8,":/CC/images/interface/chatBackground.png",this);
    chatText=new QGraphicsTextItem(this);
    configureOverlayText(chatText,QStringLiteral("#fff"));
    chatInput=new LineEdit(this);
    chatType=new ComboBox(this);
    chatType->setZValue(5);
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
    opentolan=new CustomButton(":/CC/images/interface/opentolan.png",this);
    {
        QLinearGradient gradient1( 0, 0, 0, 100 );
        gradient1.setColorAt( 0, QColor(255,255,255));
        gradient1.setColorAt( 1, QColor(255,255,255));
        QLinearGradient gradient2( 0, 0, 0, 100 );
        gradient2.setColorAt( 0, QColor(80,71,38));
        gradient2.setColorAt( 1, QColor(80,71,38));
        opentolanOver=new CustomText(gradient1,gradient2,this);
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
    options=new CustomButton(":/CC/images/interface/options.png",this);

    //on-screen D-pad + A/B (hidden until touchControlsActive). Each uses its own
    //3-state button strip (normal/pressed/disabled): the four arrow glyphs for the
    //D-pad, validate (green check) for A=action, cancel (red cross) for B. The image
    //IS the label, so no setText caption is drawn on top.
    dpadUp=new CustomButton(":/CC/images/interface/up.png",this);
    dpadDown=new CustomButton(":/CC/images/interface/down.png",this);
    dpadLeft=new CustomButton(":/CC/images/interface/left.png",this);
    dpadRight=new CustomButton(":/CC/images/interface/right.png",this);
    btnA=new CustomButton(":/CC/images/interface/validate.png",this);
    btnB=new CustomButton(":/CC/images/interface/cancel.png",this);
    dpadUp->setVisible(false);
    dpadDown->setVisible(false);
    dpadLeft->setVisible(false);
    dpadRight->setVisible(false);
    btnA->setVisible(false);
    btnB->setVisible(false);

    gainBack=new ImagesStrechMiddle(8,":/CC/images/interface/chatBackground.png",this);
    gain=new QGraphicsTextItem(gainBack);

    IG_dialog_textBack=new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this);
    IG_dialog_name=new QGraphicsTextItem(IG_dialog_textBack);
    IG_dialog_text=new QGraphicsTextItem(IG_dialog_textBack);
    configureOverlayText(IG_dialog_text,QStringLiteral("#000"));
    IG_dialog_quit=new CustomButton(":/CC/images/interface/closedialog.png",IG_dialog_textBack);

    tipBack=new ImagesStrechMiddle(8,":/CC/images/interface/chatBackground.png",this);
    tip=new QGraphicsTextItem(tipBack);

    persistant_tipBack=new ImagesStrechMiddle(8,":/CC/images/interface/chatBackground.png",this);
    persistant_tip=new QGraphicsTextItem(persistant_tipBack);

    labelSlow=new QGraphicsPixmapItem(QPixmap(":/CC/images/multi/slow.png"),this);

    if(false)
    {
        {
            gainString="gain";
            gain->setHtml(gainString);
        }
        {
            IG_dialog_nameString="Name";
            IG_dialog_name->setHtml(IG_dialog_nameString);
            IG_dialog_textString="Text";
            IG_dialog_text->setHtml(IG_dialog_textString);
        }
        {
            tipString="tip";
            tip->setHtml(tipString);
        }
        {
            persistant_tipString="persistant_tip";
            persistant_tip->setHtml(persistant_tipString);
        }
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
    if(!connect(chat,&CustomButton::clicked,this,&OverMap::updateShowChat))
        abort();
    if(!connect(IG_dialog_quit,&CustomButton::clicked,this,&OverMap::IG_dialog_close))
        abort();
    if(!connect(buy,&CustomButton::clicked,this,&OverMap::buyClicked))
        abort();
    //signal-to-signal: ScreenTransition owns the options dialog, so just relay
    if(!connect(options,&CustomButton::clicked,this,&OverMap::optionsClicked))
        abort();
    //D-pad + A/B: every pad button funnels press/release into padButtonPressed()/
    //padButtonReleased(), which read sender() to pick the Qt::Key. Held => the engine
    //keeps walking (same as a held keyboard arrow).
    if(!connect(dpadUp,&CustomButton::buttonPressed,this,&OverMap::padButtonPressed))
        abort();
    if(!connect(dpadUp,&CustomButton::buttonReleased,this,&OverMap::padButtonReleased))
        abort();
    if(!connect(dpadDown,&CustomButton::buttonPressed,this,&OverMap::padButtonPressed))
        abort();
    if(!connect(dpadDown,&CustomButton::buttonReleased,this,&OverMap::padButtonReleased))
        abort();
    if(!connect(dpadLeft,&CustomButton::buttonPressed,this,&OverMap::padButtonPressed))
        abort();
    if(!connect(dpadLeft,&CustomButton::buttonReleased,this,&OverMap::padButtonReleased))
        abort();
    if(!connect(dpadRight,&CustomButton::buttonPressed,this,&OverMap::padButtonPressed))
        abort();
    if(!connect(dpadRight,&CustomButton::buttonReleased,this,&OverMap::padButtonReleased))
        abort();
    if(!connect(btnA,&CustomButton::buttonPressed,this,&OverMap::padButtonPressed))
        abort();
    if(!connect(btnA,&CustomButton::buttonReleased,this,&OverMap::padButtonReleased))
        abort();
    if(!connect(btnB,&CustomButton::buttonPressed,this,&OverMap::padButtonPressed))
        abort();
    if(!connect(btnB,&CustomButton::buttonReleased,this,&OverMap::padButtonReleased))
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

void OverMap::setVar(CCMap *ccmap,ConnexionManager *connexionManager)
{
    this->ccmap=ccmap;
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
    playerBackground->setVisible(true);
    QPixmap pPixm=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getFrontSkinPath(informations.public_informations.skinId));
    if(playerBackgroundBig==true)
        pPixm=pPixm.scaledToHeight(pPixm.height()*2,Qt::FastTransformation);
    playerImage->setPixmap(pPixm);
    name->setHtml("<span style=\"color:#fff;\">"+QString::fromStdString(informations.public_informations.pseudo)+"<span>");
    std::cout << "info.public_informations.pseudo: " << informations.public_informations.pseudo << std::endl;
    cash->setHtml("<span style=\"color:#fff;\">"+QString::number(informations.cash)+"<span>");
    foreach (MapMonsterPreview *m, monsters)
        delete m;
    monsters.clear();
    for(const CatchChallenger::PlayerMonster &m : informations.monsters)
    {
        MapMonsterPreview *t=new MapMonsterPreview(m,this);
        if(!connect(t,&MapMonsterPreview::clicked,this,&OverMap::openMonster))
            abort();
        monsters.push_back(t);
    }
    std::pair<uint16_t,uint16_t> t=connexionManager->client->getLast_number_of_player();
    playersCount->setHtml("<span stlye=\"color:#fff;\">"+QString::number(t.first)+"<span>");
}

void OverMap::openMonster()
{
    MapMonsterPreview *m=qobject_cast<MapMonsterPreview *>(sender());
    if(monsterDetails==nullptr)
        monsterDetails=new MonsterDetails();
    monsterDetails->setVar(m->getMonster());
    emit setAbove(monsterDetails);
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
    opentolanOver->setText(tr("Open"));
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
    if(playerBackground->isVisible())
    {
        if(playerBackgroundBig==true && (w->width()<800 || w->height()<600))
        {
            playerBackgroundBig=false;
            playerBackground->setPixmap(QPixmap(":/CC/images/interface/playeruiL.png"));

            const CatchChallenger::Player_private_and_public_informations &informations=connexionManager->client->get_player_informations_ro();
            QPixmap pPixm=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getFrontSkinPath(informations.public_informations.skinId));
            playerImage->setPixmap(pPixm);

        }
        else if(playerBackgroundBig==false && w->width()>800 && w->height()>600)
        {
            playerBackgroundBig=true;
            playerBackground->setPixmap(QPixmap(":/CC/images/interface/playerui.png"));

            const CatchChallenger::Player_private_and_public_informations &informations=connexionManager->client->get_player_informations_ro();
            QPixmap pPixm=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getFrontSkinPath(informations.public_informations.skinId));
            pPixm=pPixm.scaledToHeight(pPixm.height()*2,Qt::FastTransformation);
            playerImage->setPixmap(pPixm);

        }
        if(playerBackgroundBig==true)
            playerImage->setPos(0,-10);
        else
            playerImage->setPos(0,-5);
        playerBackground->setPos(space,space);
        if(playerBackgroundBig)
        {
            name->setPos(playerBackground->x()+160,playerBackground->y()+10);
            cash->setPos(playerBackground->x()+220,playerBackground->y()+57);
            name->setVisible(true);
            cash->setVisible(true);
        }
        else
        {
            name->setVisible(false);
            cash->setVisible(false);
        }
        int tempx=playerBackground->x()+playerBackground->pixmap().width()+space;
        if(playerBackgroundBig!=true)
            tempx=playerBackground->x()+playerBackground->pixmap().width();
        unsigned int monsterIndex=0;
        while(monsterIndex<monsters.size())
        {
            MapMonsterPreview *m=monsters.at(monsterIndex);
            if(playerBackgroundBig==true)
                m->setPos(tempx,playerBackground->y());
            else
                m->setPos(tempx,0);
            tempx+=space+56;
            monsterIndex++;
        }
    }

    {
        bool showPlayersCount=!connexionManager->isLocalGame();
        #if defined(CATCHCHALLENGER_SOLO) && !defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
        if(!showPlayersCount && CatchChallenger::InternalServer::internalServer!=nullptr
           && CatchChallenger::InternalServer::internalServer->openIsOpenToLan())
            showPlayersCount=true;
        #endif
        playersCountBack->setVisible(showPlayersCount);
        playersCount->setVisible(showPlayersCount);
        if(showPlayersCount)
        {
            playersCountBack->setPos(w->width()-space-playersCountBack->pixmap().width(),space);
            playersCount->setPos(w->width()-space-80,space+15);
        }
    }

    int physicalDpiX=w->physicalDpiX();

    unsigned int xLeft=space;
    if(w->width()<800 || w->height()<600)
    {
        chat->setSize(84/2,93/2);
        chatOver->setVisible(false);
    }
    else
    {
        chat->setSize(84,93);
        chatOver->setVisible(physicalDpiX<200);
    }
    if(w->width()<800 || w->height()<600)
    {
        chatBack->setSize(200,200);
        chatInput->setFixedWidth(140);
        chatType->setFixedWidth(60);
    }
    else
    {
        chatBack->setSize(300,400);
        chatInput->setFixedWidth(200);
        chatType->setFixedWidth(100);
    }
    //on-screen D-pad (a cross to the LEFT of the chat button) — only in touch mode.
    //Resolved here (first pad use of the frame) and reused by the A/B block below.
    touchControlsActive=OverMap::touchControlsEnabled();
    //push the touch-mode state down to the map controller so a map tap is SWALLOWED in
    //pad mode. Gated at the controller's eventOnMap sink, this covers BOTH click origins
    //(the PreparedLayer tile layer AND this overlay) -- the CCMap-only gate missed the
    //tile-layer path, so a tap still moved the player.
    if(ccmap!=nullptr)
        ccmap->mapController.clickMoveDisabled=touchControlsActive;
    {
        int ps=56;
        if(w->width()<800 || w->height()<600)
            ps=42;
        dpadUp->setVisible(touchControlsActive);
        dpadDown->setVisible(touchControlsActive);
        dpadLeft->setVisible(touchControlsActive);
        dpadRight->setVisible(touchControlsActive);
        if(touchControlsActive)
        {
            dpadUp->setSize(ps,ps);
            dpadDown->setSize(ps,ps);
            dpadLeft->setSize(ps,ps);
            dpadRight->setSize(ps,ps);
            const int bottomY=w->height()-space;    //cross bottom aligned to the button row
            const int col0=xLeft;
            const int col1=xLeft+ps;
            const int col2=xLeft+2*ps;
            dpadUp->setPos(col1,bottomY-3*ps);
            dpadLeft->setPos(col0,bottomY-2*ps);
            dpadRight->setPos(col2,bottomY-2*ps);
            dpadDown->setPos(col1,bottomY-ps);
            xLeft+=3*ps+space;                        //chat sits to the RIGHT of the D-pad
        }
    }
    unsigned int chatX=xLeft;
    unsigned int chatY=w->height()-space-chat->height();
    chat->setPos(chatX,chatY);
    xLeft+=chat->width()+space;
    if(chatOver->isVisible())
    {
        chatOver->setPixelSize(18);
        chatOver->setPos(chatX+chat->width()/2-chatOver->boundingRect().width()/2,w->height()-space-chatOver->boundingRect().height());
    }
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
        //word-wrap the chat log to the chat box width (one "space" padding) so
        //server text never spills past the right edge of the box
        chatText->setTextWidth(chatBack->width()-2*static_cast<int>(space));
        chatText->setPos(space+space,y+space);
    }
    if(labelSlow->isVisible())
    {
        labelSlow->setPos(xLeft,chatY);
        xLeft+=labelSlow->pixmap().width()+space;
    }

    //compose from right to left
    unsigned int xRight=w->width()-space;
    //on-screen A/B buttons (to the RIGHT of buy) — only in touch mode. A is outermost
    //(right thumb): A=action(Enter), B=cancel(Escape). Placed first so buy/bag fall to
    //their left.
    {
        int ps=56;
        if(w->width()<800 || w->height()<600)
            ps=42;
        btnA->setVisible(touchControlsActive);
        btnB->setVisible(touchControlsActive);
        if(touchControlsActive)
        {
            btnA->setSize(ps,ps);
            btnB->setSize(ps,ps);
            const int by=w->height()-space-ps;
            btnA->setPos(xRight-ps,by);
            xRight-=ps+space;
            btnB->setPos(xRight-ps,by);
            xRight-=ps+space;
        }
    }
    if(buy->isVisible())
    {
        if(w->width()<800 || w->height()<600)
        {
            buy->setSize(84/2,93/2);
            buyOver->setVisible(false);
        }
        else
        {
            buy->setSize(84,93);
            buyOver->setVisible(physicalDpiX<200);
        }
        unsigned int buyX=xRight-buy->width();
        buy->setPos(buyX,w->height()-space-buy->height());
        xRight-=buy->width()+space;
        if(buyOver->isVisible())
        {
            buyOver->setPixelSize(18);
            buyOver->setPos(buyX+buy->width()/2-buyOver->boundingRect().width()/2,w->height()-space-buyOver->boundingRect().height());
        }
    }
    {
        if(w->width()<800 || w->height()<600)
        {
            bag->setSize(84/2,93/2);
            bagOver->setVisible(false);
        }
        else
        {
            bag->setSize(84,93);
            bagOver->setVisible(physicalDpiX<200);
        }
        unsigned int bagX=xRight-bag->width();
        bag->setPos(bagX,w->height()-space-bag->height());
        xRight-=bag->width()+space;
        if(bagOver->isVisible())
        {
            bagOver->setPixelSize(18);
            bagOver->setPos(bagX+bag->width()/2-bagOver->boundingRect().width()/2,w->height()-space-bagOver->boundingRect().height());
        }
    }
    {
        //options gear at the left of the bag: opens the options dialog in-game
        //(same art as the main menu, sized like the neighbouring map icons)
        if(w->width()<800 || w->height()<600)
            options->setSize(84/2,93/2);
        else
            options->setSize(84,93);
        options->setPos(xRight-options->width(),w->height()-space-options->height());
        xRight-=options->width()+space;
    }
    #if defined(CATCHCHALLENGER_SOLO) && ! defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    if(connexionManager->isLocalGame() && CatchChallenger::InternalServer::internalServer!=nullptr)
    {
        if(!CatchChallenger::InternalServer::internalServer->openIsOpenToLan())
        {
            opentolan->setVisible(true);
            opentolanOver->setVisible(true);
            if(w->width()<800 || w->height()<600)
            {
                opentolan->setSize(84/2,93/2);
                opentolanOver->setVisible(false);
            }
            else
            {
                opentolan->setSize(84,93);
                opentolanOver->setVisible(physicalDpiX<200);
            }
            unsigned int opentolanX=xRight-opentolan->width();
            opentolan->setPos(opentolanX,w->height()-space-opentolan->height());
            xRight-=opentolan->width()+space;
            if(opentolanOver->isVisible())
            {
                opentolanOver->setPixelSize(18);
                opentolanOver->setPos(opentolanX+opentolan->width()/2-opentolanOver->boundingRect().width()/2,w->height()-space-opentolanOver->boundingRect().height());
            }
        }
        else
        {
            opentolan->setVisible(false);
            opentolanOver->setVisible(false);
        }
    }
    else
    {
        opentolan->setVisible(false);
        opentolanOver->setVisible(false);
    }
    #endif
    Q_UNUSED(xRight);

    gainBack->setVisible(!gainString.isEmpty());
    gain->setVisible(gainBack->isVisible());
    if(gainBack->isVisible())
    {
        const QRectF &f=gain->boundingRect();
        gainBack->setSize(f.width()+space*2,f.height()+space*2);
        gainBack->setPos(w->width()/2-gainBack->width()/2,space);
        gain->setPos(space,space);
    }

    IG_dialog_textBack->setVisible(!IG_dialog_textString.isEmpty());
    IG_dialog_name->setVisible(IG_dialog_textBack->isVisible() && !IG_dialog_nameString.isEmpty());
    IG_dialog_text->setVisible(IG_dialog_textBack->isVisible());
    IG_dialog_quit->setVisible(IG_dialog_textBack->isVisible());
    if(IG_dialog_textBack->isVisible())
    {
        const int border=static_cast<int>(IG_dialog_textBack->currentBorderSize());
        const bool smallScreen=(w->width()<800 || w->height()<600);
        IG_dialog_quit->setSize(smallScreen?84/3:84/2,smallScreen?93/3:93/2);

        //Dialog box width, never wider than the window.
        int boxWidth=smallScreen?350:600;
        if(boxWidth>w->width()-2*static_cast<int>(space))
            boxWidth=w->width()-2*static_cast<int>(space);
        int innerWidth=boxWidth-2*border;
        if(innerWidth<40)
            innerWidth=40;

        //Word-wrap the name and the body to the inner width so server text can
        //never spill past the side of the dialog.
        IG_dialog_name->setTextWidth(innerWidth);
        IG_dialog_text->setTextWidth(innerWidth);
        const int nameHeight=IG_dialog_nameString.isEmpty()?0:
                    (static_cast<int>(IG_dialog_name->boundingRect().height())+static_cast<int>(space));
        const int docHeight=static_cast<int>(IG_dialog_text->boundingRect().height());

        //Box grows to fit the wrapped text but is capped to the window (so the box
        //never goes off-screen), with a per-screen-size minimum so a one-liner
        //still looks like a dialog.
        int boxHeight=2*border+nameHeight+docHeight;
        const int maxBoxHeight=w->height()-2*static_cast<int>(space);
        const int minBoxHeight=smallScreen?200:400;
        if(boxHeight<minBoxHeight)
            boxHeight=minBoxHeight;
        if(boxHeight>maxBoxHeight)
            boxHeight=maxBoxHeight;
        IG_dialog_textBack->setSize(boxWidth,boxHeight);

        const int x=w->width()/2-IG_dialog_textBack->width()/2;
        const int y=w->height()/2-IG_dialog_textBack->height()/2;
        IG_dialog_textBack->setPos(x,y);
        if(IG_dialog_nameString.isEmpty())
            IG_dialog_text->setPos(border,border);
        else
        {
            IG_dialog_name->setPos(border,border);
            IG_dialog_text->setPos(border,border+nameHeight);
        }
        IG_dialog_quit->setPos(IG_dialog_textBack->width()-IG_dialog_quit->width(),0);
    }

    int yMiddle=w->height()-space;

    persistant_tipBack->setVisible(!persistant_tipString.isEmpty());
    persistant_tip->setVisible(persistant_tipBack->isVisible());
    if(persistant_tip->isVisible())
    {
        const QRectF &f=persistant_tip->boundingRect();
        persistant_tipBack->setSize(f.width()+space*2,f.height()+space*2);

        persistant_tip->setPos(space,space);
        persistant_tipBack->setPos(w->width()/2-persistant_tipBack->width()/2,yMiddle-persistant_tipBack->height());
        yMiddle-=(f.height()+space*2+space);
    }

    tipBack->setVisible(!tipString.isEmpty());
    tip->setVisible(tipBack->isVisible());
    if(tipBack->isVisible())
    {
        const QRectF &f=tip->boundingRect();
        tipBack->setSize(f.width()+space*2,f.height()+space*2);

        tip->setPos(space,space);
        tipBack->setPos(w->width()/2-tipBack->width()/2,yMiddle-tipBack->height());
        yMiddle-=(f.height()+space*2+space);
    }
}

void OverMap::mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    chat->mousePressEventXY(p,pressValidated);
    buy->mousePressEventXY(p,pressValidated);
    bag->mousePressEventXY(p,pressValidated);
    options->mousePressEventXY(p,pressValidated);
    //NOTE: the D-pad + A/B are NOT forwarded here. They are driven by ScreenTransition
    //directly (padButtonAt() + pressAsPad()/releaseAsPad()) for BOTH mouse and touch,
    //so a held pad button is owned by its own pointer/touch and a second finger's
    //generic release can never cross-cancel it.
    #if defined(CATCHCHALLENGER_SOLO) && !defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    if(connexionManager->isLocalGame() && CatchChallenger::InternalServer::internalServer!=nullptr)
        if(!CatchChallenger::InternalServer::internalServer->openIsOpenToLan())
            opentolan->mousePressEventXY(p,pressValidated);
    #endif
    QRectF f(IG_dialog_textBack->x(),IG_dialog_textBack->y(),IG_dialog_textBack->width(),IG_dialog_textBack->height());
    if(IG_dialog_textBack->isVisible() && f.contains(p))
    {
        IG_dialog_quit->mousePressEventXY(p,pressValidated);
        pressValidated=true;
    }
    if(chat->isChecked())
    {
        const QRectF &b=QRectF(chatBack->x(),chatBack->y(),chatBack->width(),chatBack->height()+5+chatInput->height());
        const QRectF &t=mapRectToScene(b);
        if(t.contains(p))
        {
            pressValidated=true;
            callParentClass=true;
        }
        const QRectF &b2=QRectF(chatInput->x(),chatInput->y(),chatInput->width(),chatInput->height());
        const QRectF &t2=mapRectToScene(b2);
        if(!t2.contains(p))
            chatInput->clearFocus();
    }

    playerBackground->mousePressEventXY(p,pressValidated);
    for (int i = 0; i < monsters.size(); ++i)
    {
        bool previousState=pressValidated;
        monsters.at(i)->mousePressEventXY(p,pressValidated);
        if(previousState==false && pressValidated==true)
        {
            wasDragged=false;
            m_startPress=p;
            monstersDragged=monsters.at(i);
        }
    }
}

void OverMap::mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    if(monstersDragged!=nullptr)
        monstersDragged->setInDrag(false);
    monstersDragged=nullptr;
    chat->mouseReleaseEventXY(p,pressValidated);
    buy->mouseReleaseEventXY(p,pressValidated);
    bag->mouseReleaseEventXY(p,pressValidated);
    options->mouseReleaseEventXY(p,pressValidated);
    //(pad buttons intentionally not forwarded — see mousePressEventXY note)
    #if defined(CATCHCHALLENGER_SOLO) && !defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    if(connexionManager->isLocalGame() && CatchChallenger::InternalServer::internalServer!=nullptr)
        if(!CatchChallenger::InternalServer::internalServer->openIsOpenToLan())
            opentolan->mouseReleaseEventXY(p,pressValidated);
    #endif
    QRectF f(IG_dialog_textBack->x(),IG_dialog_textBack->y(),IG_dialog_textBack->width(),IG_dialog_textBack->height());
    if(IG_dialog_textBack->isVisible() && f.contains(p))
    {
        const bool alreadyConsumed=pressValidated;
        IG_dialog_quit->mouseReleaseEventXY(p,pressValidated);
        //tap on a dialog hyperlink ([Heal], quest/step links, ...): the overlay
        //receives no QGraphicsScene mouse events (input comes through this XY
        //dispatch), so QGraphicsTextItem's own link handling never fires —
        //resolve the anchor under the tap manually. The quit button sits outside
        //the text body, so both can never match the same point.
        if(!alreadyConsumed)
        {
            const QString href=IG_dialog_anchorAt(p);
            if(!href.isEmpty())
                on_IG_dialog_text_linkActivated(href);
        }
        pressValidated=true;
    }
    if(chat->isChecked())
    {
        const QRectF &b=QRectF(chatBack->x(),chatBack->y(),chatBack->width(),chatBack->height()+5+chatInput->height()+5+chat->height());
        const QRectF &t=mapRectToScene(b);
        if(t.contains(p))
        {
            pressValidated=true;
            callParentClass=true;
        }
        const QRectF &b2=QRectF(chatInput->x(),chatInput->y(),chatInput->width(),chatInput->height());
        const QRectF &t2=mapRectToScene(b2);
        if(!t2.contains(p))
            chatInput->clearFocus();
    }

    playerBackground->mouseReleaseEventXY(p,pressValidated);
    if(wasDragged)
        pressValidated=true;
    for (int i = 0; i < monsters.size(); ++i)
        monsters.at(i)->mouseReleaseEventXY(p,pressValidated);
    wasDragged=false;
    m_startPress=QPointF(0.0,0.0);
}

void OverMap::mouseMoveEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &/*callParentClass*/)
{
    qreal xDiff=p.x()-m_startPress.x();
    qreal yDiff=p.y()-m_startPress.y();
    if(xDiff<-10 || xDiff>10 || yDiff<-10 || yDiff>10)
    {
        wasDragged=true;
        if(monstersDragged!=nullptr)
        {
            bool fakeBool=true;
            monstersDragged->mouseReleaseEventXY(p,fakeBool);
            monstersDragged->setInDrag(true);
        }
    }
}

void OverMap::padButtonPressed()
{
    const int key=padKeyForSender();
    if(key!=0)
        sendKeyToMap(key,true);
}

void OverMap::padButtonReleased()
{
    const int key=padKeyForSender();
    if(key!=0)
        sendKeyToMap(key,false);
}

int OverMap::padKeyForSender()
{
    const QObject *s=sender();
    if(s==dpadUp)
        return Qt::Key_Up;
    if(s==dpadDown)
        return Qt::Key_Down;
    if(s==dpadLeft)
        return Qt::Key_Left;
    if(s==dpadRight)
        return Qt::Key_Right;
    if(s==btnA)
        return Qt::Key_Return;
    if(s==btnB)
        return Qt::Key_Escape;
    return 0;
}

void OverMap::sendKeyToMap(int key,bool pressed)
{
    if(ccmap==nullptr)
        return;
    bool eventTriggerGeneral=false;
    if(pressed)
    {
        QKeyEvent event(QEvent::KeyPress,key,Qt::NoModifier);
        ccmap->keyPressEvent(&event,eventTriggerGeneral);
    }
    else
    {
        QKeyEvent event(QEvent::KeyRelease,key,Qt::NoModifier);
        ccmap->keyReleaseEvent(&event,eventTriggerGeneral);
    }
}

CustomButton *OverMap::padButtonAt(const QPointF &scenePoint)
{
    if(!touchControlsActive)
        return nullptr;
    CustomButton *buttons[6]={dpadUp,dpadDown,dpadLeft,dpadRight,btnA,btnB};
    int i=0;
    while(i<6)
    {
        CustomButton *b=buttons[i];
        if(b!=nullptr && b->isVisible())
        {
            const QRectF t=b->mapRectToScene(b->boundingRect());
            if(t.contains(scenePoint))
                return b;
        }
        i++;
    }
    return nullptr;
}

bool OverMap::touchControlsEnabled()
{
    //cached: called every frame from paint(); the raw resolve below (QSettings +
    //QInputDevice::devices() + platform string) is too heavy for the render loop.
    //invalidateTouchControlsCache() re-arms it when the Options value changes.
    if(touchControlsCachedValue>=0)
        return touchControlsCachedValue==1;
    const int mode=Settings::settings->value("touchControls",0).toInt();
    if(mode==1)
    {
        touchControlsCachedValue=1;
        return true;
    }
    if(mode==2)
    {
        touchControlsCachedValue=0;
        return false;
    }
    //auto: on for android, an attached touch screen, or a small primary screen
    bool enabled=false;
    const QString platform=QGuiApplication::platformName();
    //NEVER auto-enable on a HEADLESS test platform (offscreen/minimal): they report no
    //input devices and an arbitrary virtual screen size, so the small-screen heuristic
    //below could wrongly flip touch mode ON and swallow every map click -- breaking the
    //channel tests that rely on click-to-move. A user can still force it (mode==1).
    if(platform==QLatin1String("offscreen") || platform==QLatin1String("minimal"))
        enabled=false;
    else if(platform==QLatin1String("android"))
        enabled=true;
    else
    {
        const QList<const QInputDevice *> devices=QInputDevice::devices();
        int i=0;
        while(i<devices.size())
        {
            const QInputDevice *dev=devices.at(i);
            if(dev!=nullptr && dev->type()==QInputDevice::DeviceType::TouchScreen)
            {
                enabled=true;
                break;
            }
            i++;
        }
        if(!enabled)
        {
            QScreen *screen=QGuiApplication::primaryScreen();
            if(screen!=nullptr)
            {
                const QSize sz=screen->size();
                if(sz.width()<800 || sz.height()<600)
                    enabled=true;
            }
        }
    }
    touchControlsCachedValue=enabled?1:0;
    return enabled;
}

void OverMap::invalidateTouchControlsCache()
{
    touchControlsCachedValue=-1;
}

void OverMap::IG_dialog_close()
{
    IG_dialog_textString.clear();
}

void OverMap::on_IG_dialog_text_linkActivated(const QString &rawlink)
{
    //no-op here: OverMapLogic overrides it with the real bot/quest link parser
    Q_UNUSED(rawlink);
}

void OverMap::IG_dialog_links_rebuild()
{
    IG_dialog_links.clear();
    IG_dialog_link_selected=-1;
    QTextBlock block=IG_dialog_text->document()->begin();
    while(block.isValid())
    {
        QTextBlock::iterator it=block.begin();
        while(!it.atEnd())
        {
            const QTextFragment fragment=it.fragment();
            if(fragment.isValid())
            {
                const QTextCharFormat format=fragment.charFormat();
                if(format.isAnchor() && !format.anchorHref().isEmpty())
                {
                    //rich text can split one <a> into several contiguous
                    //fragments: extend the previous range instead of adding a
                    //second entry for the same link
                    if(!IG_dialog_links.empty() && IG_dialog_links.back().href==format.anchorHref()
                            && IG_dialog_links.back().end==fragment.position())
                        IG_dialog_links.back().end=fragment.position()+fragment.length();
                    else
                    {
                        IG_dialog_link link;
                        link.href=format.anchorHref();
                        link.start=fragment.position();
                        link.end=fragment.position()+fragment.length();
                        IG_dialog_links.push_back(link);
                    }
                }
            }
            ++it;
        }
        block=block.next();
    }
    //GBA-style: the first link starts selected, so A/Return activates it directly
    if(!IG_dialog_links.empty())
        IG_dialog_link_selected=0;
    IG_dialog_link_highlight();
}

void OverMap::IG_dialog_link_highlight()
{
    unsigned int index=0;
    while(index<IG_dialog_links.size())
    {
        QTextCursor cursor(IG_dialog_text->document());
        cursor.setPosition(IG_dialog_links.at(index).start);
        cursor.setPosition(IG_dialog_links.at(index).end,QTextCursor::KeepAnchor);
        QTextCharFormat format;
        if(static_cast<int>(index)==IG_dialog_link_selected)
            format.setBackground(QColor(255,213,0,140));//translucent gold behind the blue link text
        else
            format.setBackground(QBrush(Qt::NoBrush));
        cursor.mergeCharFormat(format);
        index++;
    }
}

QString OverMap::IG_dialog_anchorAt(const QPointF &scenePos) const
{
    if(IG_dialog_textString.isEmpty())
        return QString();
    return IG_dialog_text->document()->documentLayout()->anchorAt(IG_dialog_text->mapFromScene(scenePos));
}

void OverMap::IG_dialog_key(const int &key)
{
    if(IG_dialog_textString.isEmpty())
        return;
    if(key==Qt::Key_Return || key==Qt::Key_Enter)
    {
        if(IG_dialog_link_selected>=0 && IG_dialog_link_selected<static_cast<int>(IG_dialog_links.size()))
            on_IG_dialog_text_linkActivated(IG_dialog_links.at(IG_dialog_link_selected).href);
        else
            IG_dialog_close();//A on a link-less dialog dismisses it, GBA-style
        return;
    }
    if(IG_dialog_links.size()<=1)
        return;//nothing to cycle
    if(key==Qt::Key_Up || key==Qt::Key_Left)
    {
        if(IG_dialog_link_selected<=0)
            IG_dialog_link_selected=static_cast<int>(IG_dialog_links.size())-1;
        else
            IG_dialog_link_selected--;
    }
    else if(key==Qt::Key_Down || key==Qt::Key_Right)
    {
        if(IG_dialog_link_selected>=static_cast<int>(IG_dialog_links.size())-1)
            IG_dialog_link_selected=0;
        else
            IG_dialog_link_selected++;
    }
    IG_dialog_link_highlight();
}

void OverMap::updateShowChat()
{
    if(chat->isChecked())
        chat->setImage(":/CC/images/interface/chat.png");
}

void OverMap::lineEdit_chat_text_returnPressed()
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
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
    if(text=="/clan_leave")
    {
        actionClan.push_back(ActionClan_Leave);
        connexionManager->client->leaveClan();
    }
    else if(text=="/clan_dissolve")
    {
        actionClan.push_back(ActionClan_Dissolve);
        connexionManager->client->dissolveClan();
    }
    else if(text.startsWith("/clan_invite "))
    {
        text.remove(0,std::string("/clan_invite ").size());
        if(!text.isEmpty())
        {
            actionClan.push_back(ActionClan_Invite);
            connexionManager->client->inviteClan(text.toStdString());
        }
    }
    else if(text.startsWith("/clan_eject "))
    {
        text.remove(0,std::string("/clan_eject ").size());
        if(!text.isEmpty())
        {
            actionClan.push_back(ActionClan_Eject);
            connexionManager->client->ejectClan(text.toStdString());
        }
    }
    else if(text.startsWith("/add_to_inventory "))
    {
        text.remove(0,std::string("/add_to_inventory ").size());
        if(!text.isEmpty())
        {
            std::vector<std::pair<uint16_t,uint32_t> > items;
            QStringList listItem = text.split(' ', Qt::SkipEmptyParts);
            int index=0;
            while(index<listItem.size())
            {
                QStringList listItemSpec = listItem.at(index).split(':', Qt::SkipEmptyParts);
                if(listItemSpec.size()==2)
                {
                    bool ok=false;
                    uint16_t item=listItemSpec.first().toUInt(&ok);
                    bool ok2=false;
                    uint16_t quantity=listItemSpec.first().toUInt(&ok2);
                    if(ok && ok2)
                        items.push_back(std::pair<uint16_t,uint32_t>(item,quantity));
                }
                index++;
            }
            if(items.size()>0)
                add_to_inventory(items,true);
        }
    }
    else if(text.startsWith("/showTip "))
    {
        text.remove(0,std::string("/showTip ").size());
        if(!text.isEmpty())
            showTip(text.toStdString());
    }
    else if(text.startsWith("/showPlace "))
    {
        text.remove(0,std::string("/showPlace ").size());
        if(!text.isEmpty())
            showPlace(text.toStdString());
    }
    else if(text=="/clan_informations")
    {
        if(!haveClanInformations)
            connexionManager->client->add_system_text(CatchChallenger::Chat_type::Chat_type_system,"No clan information");
        else if(clanName.empty() || playerInformations.clan==0)
            connexionManager->client->add_system_text(CatchChallenger::Chat_type::Chat_type_system,"No clan");
        else
            connexionManager->client->add_system_text(CatchChallenger::Chat_type::Chat_type_system,"Name: "+clanName);
    }
    else if(text.contains(QRegularExpression("^/pm [^ ]+ .+$")))
    {
        QString pseudo=text;
        pseudo.replace(QRegularExpression("^/pm ([^ ]+) .+$"), "\\1");
        text.replace(QRegularExpression("^/pm [^ ]+ (.+)$"), "\\1");
        connexionManager->client->sendPM(text.toStdString(),pseudo.toStdString());
        connexionManager->client->add_chat_text(CatchChallenger::Chat_type_pm,text.toStdString(),tr("To: ").toStdString()+pseudo.toStdString(),CatchChallenger::Player_type_normal);
    }
    else
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
    updateChat();
}

void OverMap::comboBox_chat_type_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    updateChat();
}

void OverMap::new_system_text(const CatchChallenger::Chat_type &/*chat_type*/,const std::string &/*text*/)
{
    //argument not used, see Api_protocol::add_system_text()
    updateChat();
    if(!chat->isChecked())
        chat->setImage(":/CC/images/interface/chatnew.png");
}

void OverMap::new_chat_text(CatchChallenger::Chat_type /*chat_type*/,std::string text,std::string pseudo,CatchChallenger::Player_type /*player_type*/)
{
    std::cout << "Chat::new_chat_text() pseudo=" << pseudo << " text=" << text << std::endl;
    updateChat();
    if(!chat->isChecked())
        chat->setImage(":/CC/images/interface/chatnew.png");
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
    //the chat box is laid out (and anchored to the bottom) in paint()
}

void OverMap::removeNumberForFlood()
{
    if(numberForFlood<=0)
        return;
    numberForFlood--;
}

void OverMap::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    if(chat->isChecked() && chatInput->hasFocus())
        chatInput->keyPressEvent(event);
    else
        eventTriggerGeneral=false;
}

void OverMap::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    if(chat->isChecked() && chatInput->hasFocus())
        chatInput->keyReleaseEvent(event);
    else
        eventTriggerGeneral=false;
}

void OverMap::buyClicked()
{
    QDesktopServices::openUrl(QUrl(QString::fromStdString(Ultimate::buyUrl())));
}
