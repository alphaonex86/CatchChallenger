#include "GlobalControler.h"

GlobalControler::GlobalControler(QObject *parent) :
    QObject(parent),
    settings(QCoreApplication::applicationDirPath()+"/bottest.xml",QSettings::NativeFormat)
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
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_detectSlowDown,this,&GlobalControler::detectSlowDown,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplForGui::emit_all_player_on_map,this,&GlobalControler::all_player_on_map,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnection::emit_lastReplyTime,this,&GlobalControler::lastReplyTime,Qt::QueuedConnection);
    connect(&slowDownTimer,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplForGui::detectSlowDown,Qt::QueuedConnection);
    slowDownTimer.start(200);
    connect(&autoConnect,&QTimer::timeout,this,&GlobalControler::on_connect_clicked,Qt::QueuedConnection);
    autoConnect.setSingleShot(true);
    autoConnect.start(0);
    connect(&timeoutTimer,&QTimer::timeout,this,&GlobalControler::timeoutSlot,Qt::QueuedConnection);
    timeoutTimer.setSingleShot(true);

    character_id=0;

    multipleBotConnexion.botInterface=NULL;

    if(settings.contains("login"))
        login=settings.value("login").toString();
    else
    {
        login="login";
        settings.setValue("login","login");
    }
    if(settings.contains("pass"))
        pass=settings.value("pass").toString();
    else
    {
        pass="pass";
        settings.setValue("pass","pass");
    }
    if(settings.contains("host"))
        host=settings.value("host").toString();
    else
    {
        host="localhost";
        settings.setValue("host","localhost");
    }
    if(settings.contains("port"))
        port=settings.value("port").toUInt();
    else
    {
        port=9999;
        settings.setValue("port","9999");
    }
    if(settings.contains("proxy"))
        proxy=settings.value("proxy").toString();
    else
    {
        proxy="";
        settings.setValue("proxy","");
    }
    if(settings.contains("proxyport"))
        proxyport=settings.value("proxyport").toUInt();
    else
    {
        proxyport=9999;
        settings.setValue("proxyport","9999");
    }

    if(settings.contains("charactersGroupIndex"))
        charactersGroupIndex=settings.value("charactersGroupIndex").toUInt();
    else
    {
        charactersGroupIndex=0;
        settings.setValue("charactersGroupIndex","0");
    }
    if(settings.contains("serverUniqueKey"))
        serverUniqueKey=settings.value("serverUniqueKey").toUInt();
    else
    {
        serverUniqueKey=9999;
        settings.setValue("serverUniqueKey","9999");
    }
    if(settings.contains("character"))
        character=settings.value("character").toString();
    else
    {
        character="botTest";
        settings.setValue("character","botTest");
    }

    if(settings.contains("timeout"))
        timeout=settings.value("timeout").toUInt();
    else
    {
        timeout=5*60;
        settings.setValue("timeout",5*60);
    }
    timeoutTimer.start(timeout*1000);
}

void GlobalControler::detectSlowDown(uint32_t, uint32_t worseTime)
{
    if(worseTime>7*1000)
    {
        qDebug() << "One query take more than 7000ms: " << worseTime << " (abort)";
        QCoreApplication::exit(29);
        return;
    }
}

void GlobalControler::timeoutSlot()
{
    qDebug() << "Unable to connect into the correct time (timeout): " << timeout << "s (abort)";
    QCoreApplication::exit(30);
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
    if(charactersGroupIndex>=(uint32_t)characterEntryList.size())
    {
        qDebug() << "charactersGroupIndex not found " << charactersGroupIndex << ">=" << characterEntryList.size();
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
            if(characterEntry.pseudo==character.toStdString() && characterEntry.charactersGroupIndex==charactersGroupIndex)
            {
                character_id=characterEntry.character_id;
                //multipleBotConnexion.characterSelectForFirstCharacter(characterEntry.character_id);-> do into datapackIsReady
                return;
            }
            else
                std::cerr << "character not match: \"" << characterEntry.pseudo << "\"!=\"" << character.toStdString() << "\", charactersGroupIndex: " << std::to_string(characterEntry.charactersGroupIndex) << "!=" << std::to_string(charactersGroupIndex) << std::endl;
            index++;
        }
        {
            std::cerr << "character not found: \"" << character.toStdString() << "\", charactersGroupIndex: " << std::to_string(charactersGroupIndex) << std::endl;
            int index=0;
            QList<CatchChallenger::CharacterEntry> characterEntryListNew=characterEntryList.at(charactersGroupIndex);
            while(index<characterEntryListNew.size())
            {
                const CatchChallenger::CharacterEntry &characterEntry=characterEntryListNew.at(index);
                std::cout << "- character: \"" << characterEntry.pseudo << "\", id: " << characterEntry.character_id << ", charactersGroupIndex: " << std::to_string(characterEntry.charactersGroupIndex);
                index++;
            }
            QCoreApplication::exit(25);
            return;
        }
    }
}

void GlobalControler::updateServerList(CatchChallenger::Api_client_real *)
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
            {
                //multipleBotConnexion.serverSelect(characterEntry.character_id);-> do into datapackIsReady
                return;
            }
            index++;
        }
    }
    qDebug() << "server or charactersGroupIndex not found";
    {
        int index=0;
        while(index<serverOrdenedList.size())
        {
            const CatchChallenger::ServerFromPoolForDisplay &server=*serverOrdenedList.at(index);
            qDebug() << server.name << " (" << server.host << ":" << server.port << "), charactersGroupIndex: " << server.charactersGroupIndex << ", unique key: " << server.uniqueKey;
            index++;
        }
    }
    QCoreApplication::exit(23);
}

void GlobalControler::lastReplyTime(const quint32 &time)
{
    if(time>5000)
        qDebug() << tr("Last reply time: %1ms").arg(time);
}

void GlobalControler::statusError(QString error)
{
    qDebug() << error;
    QCoreApplication::exit(26);
}

void GlobalControler::on_connect_clicked()
{
    qDebug() << "GlobalControler::on_connect_clicked()";
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

    QDir datapack(QCoreApplication::applicationDirPath()+"/datapack/");
    if(!datapack.exists())
        if(!datapack.mkpath(datapack.absolutePath()))
        {
            std::cerr << "Unable to create the datapack folder: " << datapack.absolutePath().toStdString() << std::endl;
            abort();
        }

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
    QCoreApplication::exit(0);
}

void GlobalControler::all_player_on_map()
{
    qDebug() << "GlobalControler::all_player_on_map()";
}

