#include "SimpleAction.h"
#include "../../general/base/CommonSettingsServer.h"

SimpleAction::SimpleAction()
{
    connect(&moveTimer,&QTimer::timeout,this,&SimpleAction::doMove);
    connect(&textTimer,&QTimer::timeout,this,&SimpleAction::doText);
    moveTimer.start(1000);
    textTimer.start(1000);
    purgeCpuCache();
}

SimpleAction::~SimpleAction()
{
}

void SimpleAction::insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(player);
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);

    SimpleBotInterface::insert_player(api,player,mapId,x,y,direction);
    connect(api,&CatchChallenger::Api_protocol::new_chat_text,this,&SimpleAction::new_chat_text,Qt::QueuedConnection);
}

void SimpleAction::purgeCpuCache()
{
    const int size=16*1024*1024;
    char *var=static_cast<char *>(malloc(size));
    memset(var,0,size);
    memcmp(var,var,size);
    delete var;
}

void SimpleAction::doMove()
{
    if(!move)
        return;

    purgeCpuCache();
    QHashIterator<CatchChallenger::Api_protocol *,Player> i(clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        Player &player=clientList[i.key()];
        //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
        if(api->getCaracterSelected())
        {
            if(bugInDirection)
                api->send_player_move(0,player.direction);
            else
            {
                if(player.direction==CatchChallenger::Direction_look_at_bottom)
                {
                    player.direction=CatchChallenger::Direction_look_at_left;
                    api->send_player_move(0,player.direction);
                }
                else if(player.direction==CatchChallenger::Direction_look_at_left)
                {
                    player.direction=CatchChallenger::Direction_look_at_top;
                    api->send_player_move(0,player.direction);
                }
                else if(player.direction==CatchChallenger::Direction_look_at_top)
                {
                    player.direction=CatchChallenger::Direction_look_at_right;
                    api->send_player_move(0,player.direction);
                }
                else if(player.direction==CatchChallenger::Direction_look_at_right)
                {
                    player.direction=CatchChallenger::Direction_look_at_bottom;
                    api->send_player_move(0,player.direction);
                }
                else
                {
                    qDebug() << "Out of direction scope";
                    abort();
                }
            }
        }
    }
}

void SimpleAction::doText()
{
    if(!randomText)
        return;
    if(clientList.isEmpty())
        return;

    purgeCpuCache();
    QList<CatchChallenger::Api_protocol *> clientListApi;
    QHashIterator<CatchChallenger::Api_protocol *,Player> i(clientList);
    while (i.hasNext()) {
        i.next();
        clientListApi << i.key();
    }
    CatchChallenger::Api_protocol *api=clientListApi.at(rand()%clientListApi.size());
    //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
    if(api->getCaracterSelected())
    {
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local && rand()%10==0)
        {
            switch(rand()%3)
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"What's up?");
                break;
                case 1:
                    api->sendChatText(CatchChallenger::Chat_type_local,"Have good day!");
                break;
                case 2:
                    api->sendChatText(CatchChallenger::Chat_type_local,"... and now, what I have win :)");
                break;
            }
        }
        else
        {
            if(CommonSettingsServer::commonSettingsServer.chat_allow_all && rand()%100==0)
            {
                switch(rand()%4)
                {
                    case 0:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Hello world! :)");
                    break;
                    case 1:
                        api->sendChatText(CatchChallenger::Chat_type_all,"It's so good game!");
                    break;
                    case 2:
                        api->sendChatText(CatchChallenger::Chat_type_all,"This game have reason to ask donations!");
                    break;
                    case 3:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Donate if you can!");
                    break;
                }
            }
        }
    }
}

void SimpleAction::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type)
{
    if(!globalChatRandomReply && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    purgeCpuCache();
    Q_UNUSED(text);
    Q_UNUSED(pseudo);
    Q_UNUSED(type);
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;

    Q_UNUSED(type);
    switch(chat_type)
    {
        case CatchChallenger::Chat_type_all:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_all)
            switch(rand()%(100*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"I'm according "+pseudo);
                break;
                default:
                break;
            }
        break;
        case CatchChallenger::Chat_type_local:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local)
            switch(rand()%(3*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"You are in right "+pseudo);
                break;
            }
        break;
        case CatchChallenger::Chat_type_pm:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_private)
        {
            if(text==QStringLiteral("version"))
                api->sendPM(QStringLiteral("Version %1 %2").arg(name()).arg(version()),pseudo);
            else
                api->sendPM(QStringLiteral("Hello %1, I'm few bit busy for now").arg(pseudo),pseudo);
        }
        break;
        default:
        break;
    }
}

