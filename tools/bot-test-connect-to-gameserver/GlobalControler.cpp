#include "GlobalControler.h"

GlobalControler::GlobalControler(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<QList<QList<CatchChallenger::CharacterEntry> > >("QList<QList<CatchChallenger::CharacterEntry> >");
    qRegisterMetaType<QList<CatchChallenger::ServerFromPoolForDisplay*> >("QList<CatchChallenger::ServerFromPoolForDisplay*>");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::loggedDone,this,&GlobalControler::logged,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackIsReady,this,&GlobalControler::datapackIsReady,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::datapackMainSubIsReady,this,&GlobalControler::datapackMainSubIsReady,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::statusError,this,&GlobalControler::statusError,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfSelectedCharacter,this,&GlobalControler::display_numberOfSelectedCharacter,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_numberOfBotConnected,this,&GlobalControler::display_numberOfBotConnected);//,Qt::QueuedConnection
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_detectSlowDown,this,&GlobalControler::detectSlowDown,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_connected,this,&GlobalControler::all_player_connected,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_on_map,this,&GlobalControler::all_player_on_map,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnection::emit_lastReplyTime,this,&GlobalControler::lastReplyTime,Qt::QueuedConnection);
    connect(&slowDownTimer,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplForGui::detectSlowDown,Qt::QueuedConnection);
    slowDownTimer.start(200);
    connect(&autoConnect,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplForGui::on_connect_clicked,Qt::QueuedConnection);
    autoConnect.setSingleShot(true);
    autoConnect.start(0);
    character_id=0;

    multipleBotConnexion.botInterface=NULL;

    if(settings.contains("login"))
        login=settings.value("login").toString();
    else
        settings.setValue("login","login");
    if(settings.contains("pass"))
        pass=settings.value("pass").toString();
    else
        settings.setValue("pass","pass");
    if(settings.contains("host"))
        host=settings.value("host").toString();
    else
        settings.setValue("host","localhost");
    if(settings.contains("port"))
        port=settings.value("port").toUInt();
    else
        settings.setValue("port","9999");
    if(settings.contains("proxy"))
        proxy=settings.value("proxy").toString();
    else
        settings.setValue("proxy","");
    if(settings.contains("proxyport"))
        proxyport=settings.value("proxyport").toUInt();
    else
        settings.setValue("proxyport","9999");

    if(settings.contains("charactersGroupIndex"))
        charactersGroupIndex=settings.value("charactersGroupIndex").toUInt();
    else
        settings.setValue("charactersGroupIndex","0");
    if(settings.contains("serverUniqueKey"))
        serverUniqueKey=settings.value("serverUniqueKey").toUInt();
    else
        settings.setValue("serverUniqueKey","9999");
    if(settings.contains("character"))
        character=settings.value("character").toUInt();
    else
        settings.setValue("character","botTest");
}

void GlobalControler::detectSlowDown(QString text)
{
//    ui->labelQueryList->setText(text);
}

void GlobalControler::logged(CatchChallenger::Api_client_real *senderObject,const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList,bool haveTheDatapack)
{
    Q_UNUSED(haveTheDatapack);
    if(senderObject==NULL)
    {
        qDebug() << "GlobalControler::logged(): qobject_cast<CatchChallenger::Api_client_real *>(sender())==NULL";
        return;
    }

    this->serverOrdenedList=serverOrdenedList;
    this->characterEntryList=characterEntryList;
    if(characterEntryList.size()>=charactersGroupIndex)
    {
        qDebug() << "charactersGroupIndex not found";
        QCoreApplication::exit(24);
        return;
    }
    updateServerList(senderObject);
    {
        int index=0;
        QList<CatchChallenger::CharacterEntry> characterEntryListNew=characterEntryList.at(charactersGroupIndex);
        while(index<characterEntryListNew.size())
        {
            const CatchChallenger::CharacterEntry &characterEntry=characterEntryListNew.at(index);
            if(characterEntry.pseudo==character)
            {
                character_id=characterEntry.character_id;
                multipleBotConnexion.characterSelectForFirstCharacter(characterEntry.character_id);
                return;
            }
            index++;
        }
        {
            qDebug() << "character not found";
            QCoreApplication::exit(25);
            return;
        }
    }
}

void GlobalControler::updateServerList(CatchChallenger::Api_client_real *senderObject)
{
    //do the grouping for characterGroup count
    {
        serverByCharacterGroup.clear();
        int index=0;
        int serverByCharacterGroupTempIndexToDisplay=1;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=*serverOrdenedList.at(index);
            if(serverByCharacterGroup.contains(server.charactersGroupIndex))
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
    {
        int index=0;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=*serverOrdenedList.at(index);
            if(server.charactersGroupIndex==charactersGroupIndex && server.uniqueKey==serverUniqueKey)
                return;
            index++;
        }
    }
    qDebug() << "server or charactersGroupIndex not found";
    QCoreApplication::exit(23);
}

void GlobalControler::statusError(QString error)
{
    qDebug() << error;
    QCoreApplication::exit(26);
}

void GlobalControler::on_connect_clicked()
{
    if(pass.size()<6)
    {
        qDebug() << "Your password need to be at minimum of 6 characters";
        QCoreApplication::exit(27);
        return;
    }
    if(login.size()<3)
    {
        qDebug() << "Your login need to be at minimum of 3 characters";
        QCoreApplication::exit(28);
        return;
    }

    multipleBotConnexion.mLogin=login;
    multipleBotConnexion.mPass=pass;
    multipleBotConnexion.mMultipleConnexion=false;
    multipleBotConnexion.mAutoCreateCharacter=false;
    multipleBotConnexion.mConnectBySeconds=9;
    multipleBotConnexion.mConnexionCountTarget=9;
    multipleBotConnexion.mMaxDiffConnectedSelected=9;
    multipleBotConnexion.mProxy=proxy;
    multipleBotConnexion.mProxyport=proxyport;
    multipleBotConnexion.mHost=host;
    multipleBotConnexion.mPort=port;

    //do only the first client to download the datapack
    multipleBotConnexion.createClient();
}

void GlobalControler::datapackIsReady()
{
    qDebug() << "GlobalControler::datapackIsReady()";
    multipleBotConnexion.serverSelect(charactersGroupIndex,serverUniqueKey);
    multipleBotConnexion.characterSelectForFirstCharacter(character_id);
}

void GlobalControler::datapackMainSubIsReady()
{
    qDebug() << "GlobalControler::datapackMainSubIsReady()";
}

void GlobalControler::all_player_connected()
{
    qDebug() << "GlobalControler::all_player_connected()";
}

void GlobalControler::all_player_on_map()
{
    qDebug() << "GlobalControler::all_player_on_map()";
    QCoreApplication::exit(0);
}

