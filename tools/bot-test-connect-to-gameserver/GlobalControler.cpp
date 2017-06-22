#include "GlobalControler.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLibGeneral.h"

#include <iostream>
#include <stdlib.h>

GlobalControler::GlobalControler(QObject *parent) :
    QObject(parent),
    settings(QCoreApplication::applicationDirPath()+"/bottest.xml",QSettings::NativeFormat)
{
    srand(time(NULL));

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
    char_count=99999999;

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
    std::cout << "GlobalControler::logged()" << std::endl;

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
        CatchChallenger::Api_client_real * api=NULL;
        QHashIterator<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> i(multipleBotConnexion.apiToCatchChallengerClient);
        if(i.hasNext())
        {
            i.next();
            api=i.value()->api;
        }
        else
        {
            qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): BUG: no api";
            return;
        }

        //char count
        {
            char_count=0;
            unsigned int charactersGroupIndexTemp=0;
            while(charactersGroupIndexTemp<(unsigned int)characterEntryList.size())
            {
                char_count+=characterEntryList.at(charactersGroupIndexTemp).size();
                charactersGroupIndexTemp++;
            }
        }

        QList<CatchChallenger::CharacterEntry> characterEntryListNew=characterEntryList.at(charactersGroupIndex);
        //delete char to test
        if(rand()%100<20)
        {
            std::cout << "\e[1mdelete character triggered\e[0m" << std::endl;
            int index=0;
            while(index<characterEntryListNew.size())
            {
                const CatchChallenger::CharacterEntry &characterEntry=characterEntryListNew.at(index);
                if(!(characterEntry.pseudo==character.toStdString() && characterEntry.charactersGroupIndex==charactersGroupIndex) && characterEntry.delete_time_left==0)
                {
                    std::cerr << "delete character: \"" << characterEntry.pseudo << "\"!=\"" << character.toStdString() << "\", charactersGroupIndex: " << std::to_string(characterEntry.charactersGroupIndex) << "!=" << std::to_string(charactersGroupIndex) << std::endl;
                    api->removeCharacter(charactersGroupIndex,characterEntry.character_id);
                    break;
                }
                index++;
            }
        }
        //select char
        int index=0;
        while(index<characterEntryListNew.size())
        {
            const CatchChallenger::CharacterEntry &characterEntry=characterEntryListNew.at(index);
            if(characterEntry.pseudo==character.toStdString() && characterEntry.charactersGroupIndex==charactersGroupIndex)
            {
                character_id=characterEntry.character_id;
                if(character_id==0)
                {
                    std::cerr << "GlobalControler::logged()) character_id==0" << std::endl;
                    abort();
                }
                std::cout << "character_id set for:" << character_id << ", charactersGroupIndex: " << charactersGroupIndex << std::endl;
                //multipleBotConnexion.characterSelectForFirstCharacter(characterEntry.character_id);-> do into datapackIsReady
                return;
            }
            else
                std::cerr << "character not match: \"" << characterEntry.pseudo << "\"!=\"" << character.toStdString() << "\", charactersGroupIndex: " << std::to_string(characterEntry.charactersGroupIndex) << "!=" << std::to_string(charactersGroupIndex) << std::endl;
            index++;
        }

        //not found
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
    std::cout << "GlobalControler::updateServerList()" << std::endl;
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
                if(CatchChallenger::CommonDatapack::commonDatapack.isParsedContent())
                {
                    multipleBotConnexion.serverSelect(charactersGroupIndex,serverUniqueKey);
                    if(character_id!=0)
                        multipleBotConnexion.characterSelectForFirstCharacter(character_id);
                    else
                        std::cerr << "GlobalControler::datapackIsReady() character_id==0" << std::endl;
                }
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
    if(character_id==0)
        std::cerr << "GlobalControler::datapackIsReady() character_id==0" << std::endl;
    else
        multipleBotConnexion.characterSelectForFirstCharacter(character_id);

    //add char to test, need have datapack loaded before all
    if(rand()%100<20)
    {
        if(char_count<CommonSettingsCommon::commonSettingsCommon.max_character)
        {
            CatchChallenger::Api_client_real * api=NULL;
            QHashIterator<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> i(multipleBotConnexion.apiToCatchChallengerClient);
            if(i.hasNext())
            {
                i.next();
                api=i.value()->api;
            }
            else
            {
                qDebug() << "MultipleBotConnectionImplFoprGui::characterSelect(): BUG: no api";
                return;
            }

            if(!CatchChallenger::CommonDatapack::commonDatapack.isParsedContent())
                std::cerr << "CatchChallenger::CommonDatapack::commonDatapack.isParsedContent() then unable to create character" << std::endl;
            else if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
                std::cerr << "Profile list is empty" << std::endl;
            else
            {
                std::cout << "\e[1madd character triggered\e[0m" << std::endl;
                //load the datapack
                QFileInfoList skinsList;
                {
                    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack((QCoreApplication::applicationDirPath()+"/datapack/").toStdString());
                    //load the skins list
                    QDir dir(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/skin/fighter/"));
                    skinsList=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
                }

                qDebug() << "create new character";
                quint8 profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.profileList.size();
                QString pseudo="bot"+QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettingsCommon::commonSettingsCommon.max_pseudo_size-3));
                uint8_t skinId;
                const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.profileList.at(profileIndex);
                if(!profile.forcedskin.empty())
                    skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
                else
                    skinId=rand()%skinsList.size();
                uint8_t monstergroupId=rand()%profile.monstergroup.size();
                api->addCharacter(charactersGroupIndex,profileIndex,pseudo,monstergroupId,skinId);
            }
        }
    }
}

void GlobalControler::datapackMainSubIsReady()
{
    qDebug() << "\e[1mGlobalControler::datapackMainSubIsReady()\e[0m";
    QCoreApplication::exit(0);
}

void GlobalControler::all_player_on_map()
{
    qDebug() << "GlobalControler::all_player_on_map()";
}

