#include "Api_protocol_Qt.hpp"
#include "../Language.hpp"
#include "../../qt/fight/interface/ClientFightEngine.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#ifndef NOTCPSOCKET
//#include "SslCert.h"
#include <QSslKey>
#endif
#include <iostream>
#include <cstring>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>

using namespace CatchChallenger;

Api_protocol_Qt::Api_protocol_Qt(ConnectedSocket *socket)
{
    //register meta type
    #ifndef EPOLLCATCHCHALLENGERSERVER
        qRegisterMetaType<CatchChallenger::PlayerMonster >("CatchChallenger::PlayerMonster");//for Api_protocol_Qt::tradeAddTradeMonster()
        qRegisterMetaType<PublicPlayerMonster >("PublicPlayerMonster");//for battleAcceptedByOther(stat,publicPlayerMonster);
        qRegisterMetaType<std::vector<uint8_t> >("std::vector<uint8_t>");//for battleAcceptedByOther(stat,publicPlayerMonster);
        qRegisterMetaType<std::vector<Skill::AttackReturn> >("std::vector<Skill::AttackReturn>");//for battleAcceptedByOther(stat,publicPlayerMonster);
        qRegisterMetaType<std::vector<CharacterEntry> >("std::vector<CharacterEntry>");
        qRegisterMetaType<std::vector<uint8_t> >("std::vector<uint8_t>");//for battleAcceptedByOther(stat,publicPlayerMonster);
        qRegisterMetaType<std::vector<Skill::AttackReturn> >("std::vector<Skill::AttackReturn>");//for battleAcceptedByOther(stat,publicPlayerMonster);
        qRegisterMetaType<std::vector<CharacterEntry> >("std::vector<CharacterEntry>");
        #ifndef __EMSCRIPTEN__
            #if ! defined (ONLYMAPRENDER)
                qRegisterMetaType<QSslSocket::SslMode>("QSslSocket::SslMode");
            #endif
        #endif
    #endif

    this->socket=socket;
    if(!QObject::connect(socket,&ConnectedSocket::destroyed,this,&Api_protocol_Qt::QtsocketDestroyed))
        abort();
    #ifndef NOTCPSOCKET
    if(socket->sslSocket!=NULL)
    {
        if(!QObject::connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol_Qt::readForFirstHeader))
            abort();
        if(socket->bytesAvailable())
            readForFirstHeader();
    }
    else
    #endif
    #ifndef NOWEBSOCKET
    if(socket->webSocket!=NULL)
    {
        if(!QObject::connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol_Qt::readForFirstHeader))
            abort();
        if(socket->bytesAvailable())
            readForFirstHeader();
    }
    else
    #endif
    {
        #ifndef NOTCPSOCKET
        #ifdef CATCHCHALLENGER_SOLO
        if(socket->fakeSocket!=NULL)
            haveFirstHeader=true;
        #endif
        #endif
        if(!QObject::connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol_Qt::parseIncommingData,Qt::QueuedConnection))//put queued to don't have circular loop Client -> Server -> Client
            abort();
        if(socket->bytesAvailable())
            parseIncommingData();
    }
    if(!connect(socket,&ConnectedSocket::stateChanged,    this,&Api_protocol_Qt::stateChanged,Qt::QueuedConnection))//Qt::QueuedConnection mandatory, Qt::DirectConnection do wrong order call problem (disconnect before set stage 2 + new token to go on game server)
        abort();

    fightEngine=new ClientFightEngine();
/*    if(!connect(fightEngine,&Api_protocol_Qt::newError,  this,&Api_protocol_Qt::newError))
        abort();
    if(!connect(fightEngine,&Api_protocol_Qt::error,     this,&Api_protocol_Qt::errorFromFightEngine))
        abort();
    if(!connect(fightEngine,&Api_protocol_Qt::message,     this,&Api_protocol_Qt::message))
        abort();*/
    /*if(!connect(fightEngine,&Api_protocol_Qt::errorFightEngine,     this,&BaseWindow::stderror))
        abort();*/
    if(!connect(this,&Api_protocol_Qt::Qtrandom_seeds,this,&Api_protocol_Qt::newRandomNumber))
           abort();
    fightEngine->setClient(this);
}

Api_protocol_Qt::~Api_protocol_Qt()
{
    delete fightEngine;
}

void Api_protocol_Qt::stateChanged(QAbstractSocket::SocketState socketState)
{
    std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ")" << std::endl;
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ") client!=NULL, client->stage(): " << stage() << std::endl;
        if(stage()==StageConnexion::Stage2 || stage()==StageConnexion::Stage3)
        {
            std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ") call socketDisconnectedForReconnect" << std::endl;
            socketDisconnectedForReconnect();//need by call after closeSocket() because it call the reconnection
            return;
        }
    }
}

void Api_protocol_Qt::errorFromFightEngine(const std::string &error)
{
    emit QtnewError(error,error);
}

void Api_protocol_Qt::parseIncommingData()
{
    Api_protocol::parseIncommingData();
}

bool Api_protocol_Qt::disconnectClient()
{
    if(socket!=NULL)
        socket->disconnect();
    return Api_protocol::disconnectClient();
}

void Api_protocol_Qt::tryDisconnect()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

void Api_protocol_Qt::disconnectFromHost()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

void Api_protocol_Qt::QtsocketDestroyed()
{
    socketDestroyed();
}

void Api_protocol_Qt::socketDestroyed()
{
    socket=NULL;
    Api_protocol::socketDestroyed();
}

void Api_protocol_Qt::hashSha224(const char * const data,const int size,char *buffer)
{
    QCryptographicHash hash(QCryptographicHash::Sha224);
    hash.addData(data,size);
    memcpy(buffer,hash.result(),CATCHCHALLENGER_SHA224HASH_SIZE);
}

void Api_protocol_Qt::connectTheExternalSocketInternal()
{
    #ifndef NOTCPSOCKET
    if(socket->sslSocket!=NULL)
    {
        if(socket->peerName().isEmpty() || socket->sslSocket->state()!=QSslSocket::SocketState::ConnectedState)
        {
            newError(std::string("Internal problem"),std::string("Api_protocol_Qt::connectTheExternalSocket() socket->sslSocket->peerAddress()==QHostAddress::Null: ")+
                     socket->peerName().toStdString()+"-"+std::to_string(socket->peerPort())+
                     ", state: "+std::to_string(socket->sslSocket->state())
                     );
            return;
        }
        //check the certificat
        {
            QDir datapackCert(QString("%1/cert/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)));
            datapackCert.mkpath(datapackCert.absolutePath());
            QFile certFile;
            if(stageConnexion==StageConnexion::Stage1)
                certFile.setFileName(datapackCert.absolutePath()+"/"+socket->peerName()+"-"+QString::number(socket->peerPort()));
            else if(stageConnexion==StageConnexion::Stage3 || stageConnexion==StageConnexion::Stage4)
            {
                if(selectedServerIndex==-1)
                {
                    parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("Api_protocol_Qt::connectTheExternalSocket() selectedServerIndex==-1"));
                    return;
                }
                const ServerFromPoolForDisplay &serverFromPoolForDisplay=serverOrdenedList.at(selectedServerIndex);
                certFile.setFileName(
                            datapackCert.absolutePath()+QString("/")+
                                     QString::fromStdString(serverFromPoolForDisplay.host)+QString("-")+
                            QString::number(serverFromPoolForDisplay.port)
                            );
            }
            else
            {
                parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("Api_protocol_Qt::connectTheExternalSocket() stageConnexion!=StageConnexion::Stage1/3"));
                return;
            }
            if(certFile.exists())
            {
                if(socket->sslSocket->mode()==QSslSocket::UnencryptedMode)
                {
                    #if (!defined(CATCHCHALLENGER_VERSION_SOLO) || defined(CATCHCHALLENGER_MULTI)) && ! defined(BOTTESTCONNECT)
                    /*SslCert sslCert(NULL);
                    sslCert.exec();
                    if(sslCert.validated())
                        saveCert(certFile.fileName().toStdString());
                    else
                    {
                        socket->sslSocket->disconnectFromHost();
                        return;
                    }*/
                    #endif
                }
                else if(certFile.open(QIODevice::ReadOnly))
                {
                    if(socket->sslSocket->peerCertificate().publicKey().toPem()!=certFile.readAll())
                    {
                        #if (!defined(CATCHCHALLENGER_VERSION_SOLO) || defined(CATCHCHALLENGER_MULTI)) && ! defined(BOTTESTCONNECT)
                        /*SslCert sslCert(NULL);
                        sslCert.exec();
                        if(sslCert.validated())
                            saveCert(certFile.fileName().toStdString());
                        else
                        {
                            socket->sslSocket->disconnectFromHost();
                            return;
                        }*/
                        #endif
                    }
                    certFile.close();
                }
            }
            else
            {
                if(socket->sslSocket->mode()!=QSslSocket::UnencryptedMode)
                    saveCert(certFile.fileName().toStdString());

            }
        }
    }
    /*else
    {
        newError(std::string("Internal problem"),std::string("Api_protocol_Qt::connectTheExternalSocket() socket->sslSocket==NULL"));
        return;
    }*/
    #endif
    //continue the normal procedure
    Api_protocol::connectTheExternalSocketInternal();

    if(stageConnexion==StageConnexion::Stage1)
        if(!QObject::connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol_Qt::parseIncommingData,Qt::QueuedConnection))//put queued to don't have circular loop Client -> Server -> Client
            abort();
    if(socket->bytesAvailable())
        parseIncommingData();
}

#ifndef NOTCPSOCKET
void Api_protocol_Qt::saveCert(const std::string &file)
{
    if(socket->sslSocket==NULL)
        return;
    QFile certFile(QString::fromStdString(file));
    if(socket->sslSocket->mode()==QSslSocket::UnencryptedMode)
        certFile.remove();
    else
    {
        if(certFile.open(QIODevice::WriteOnly))
        {
            qDebug() << "Register the certificate into" << certFile.fileName();
            /*qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::Organization);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::CommonName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::LocalityName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::OrganizationalUnitName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::CountryName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::StateOrProvinceName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::EmailAddress);*/
            certFile.write(socket->sslSocket->peerCertificate().publicKey().toPem());
            certFile.close();
        }
    }
}
#endif

void Api_protocol_Qt::useSeed(const uint8_t &plant_id)
{
    Api_protocol::useSeed(plant_id);
}

void Api_protocol_Qt::collectMaturePlant()
{
    Api_protocol::collectMaturePlant();
}

void Api_protocol_Qt::destroyObject(const uint16_t &object,const uint32_t &quantity)
{
    Api_protocol::destroyObject(object,quantity);
}

void Api_protocol_Qt::readForFirstHeader()
{
    if(haveFirstHeader)
        return;
    #if ! defined(NOTCPSOCKET) && ! defined(NOWEBSOCKET)
    if(socket->sslSocket==NULL && socket->webSocket==NULL)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol_Qt::readForFirstHeader() socket->sslSocket==NULL"));
        return;
    }
    #else
        #ifndef NOTCPSOCKET
        if(socket->sslSocket==NULL)
        {
            newError(std::string("Internal problem"),std::string("Api_protocol_Qt::readForFirstHeader() socket->sslSocket==NULL"));
            return;
        }
        #endif
        #ifndef NOWEBSOCKET
        if(socket->webSocket==NULL)
        {
            newError(std::string("Internal problem"),std::string("Api_protocol_Qt::readForFirstHeader() socket->sslSocket==NULL"));
            return;
        }
        #endif
    #endif
    if(stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2 && stageConnexion!=StageConnexion::Stage3)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol_Qt::readForFirstHeader() stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2"));
        return;
    }
    if(stageConnexion==StageConnexion::Stage2)
    {
        message("stageConnexion=CatchChallenger::Api_protocol_Qt::StageConnexion::Stage3 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        stageConnexion=StageConnexion::Stage3;
    }
    {
        #ifndef NOTCPSOCKET
        if(socket->sslSocket!=NULL && socket->sslSocket->mode()!=QSslSocket::UnencryptedMode)
        {
            newError(std::string("Internal problem"),std::string("socket->sslSocket->mode()!=QSslSocket::UnencryptedMode into Api_protocol_Qt::readForFirstHeader()"));
            return;
        }
        #endif
        uint8_t value;
        if(socket!=NULL && socket->read((char*)&value,sizeof(value))==sizeof(value))
        {
            haveFirstHeader=true;
            if(value==0x01)
            {
                #ifndef NOTCPSOCKET
                if(socket->sslSocket!=NULL)
                {
                    socket->sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
                    socket->sslSocket->ignoreSslErrors();
                    socket->sslSocket->startClientEncryption();
                    if(!QObject::connect(socket->sslSocket,&QSslSocket::encrypted,this,&Api_protocol_Qt::sslHandcheckIsFinished))
                        abort();
                }
                else
                    newError(std::string("Internal problem"),std::string("socket->sslSocket->mode()!=QSslSocket::UnencryptedMode into Api_protocol_Qt::readForFirstHeader()"));
                #else
                newError(std::string("Internal problem"),std::string("socket->sslSocket->mode()!=QSslSocket::UnencryptedMode into Api_protocol_Qt::readForFirstHeader()"));
                #endif
            }
            else
                connectTheExternalSocketInternal();
        }
    }
}

void Api_protocol_Qt::sslHandcheckIsFinished()
{
     Api_protocol::sslHandcheckIsFinished();
}

void Api_protocol_Qt::resetAll()
{
    messageParsingLayer("Api_protocol::resetAll(): stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage1 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
    #ifndef NOTCPSOCKET
    #ifdef CATCHCHALLENGER_SOLO
    if(socket!=NULL && socket->fakeSocket!=NULL)
        haveFirstHeader=true;
    else
    #endif
    #else
    if(socket==NULL)
        haveFirstHeader=true;
    else
    #endif
    mDatapackBase=QCoreApplication::applicationDirPath().toStdString()+"/datapack/";
    #ifdef Q_OS_ANDROID
    mDatapackBase=QStandardPaths::writableLocation(QStandardPaths::DataLocation).toStdString()+"/datapack/";
    #endif
    mDatapackMain=mDatapackBase+"map/main/[main]/";
    mDatapackSub=mDatapackMain+"sub/[sub]/";
    Api_protocol::resetAll();

    mLastGivenXP=0;
    fightId=0;

    randomSeeds.clear();
    player_informations_local.playerMonster.clear();
    fightEffectList.clear();

    battleCurrentMonster.clear();
    battleStat.clear();
    client=NULL;

    CommonFightEngine::resetAll();
}

bool Api_protocol_Qt::tryLogin(const std::string &login, const std::string &pass)
{
    std::string peerName=socket->peerName().toStdString();
    if(peerName.size()>255)
    {
        newError(QObject::tr("Hostname too big").toStdString(),std::string("Hostname too big"));
        return false;
    }
    return Api_protocol::tryLogin(login,pass);
}

std::string Api_protocol_Qt::socketDisconnectedForReconnect()
{
    std::string returnVar=Api_protocol::socketDisconnectedForReconnect();
    const ServerFromPoolForDisplay &serverFromPoolForDisplay=serverOrdenedList.at(selectedServerIndex);
    if(socket==NULL)
    {
        parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("socket==NULL with Api_protocol_Qt::socketDisconnectedForReconnect()"));
        return serverFromPoolForDisplay.host+":"+std::to_string(serverFromPoolForDisplay.port);
    }
    socket->connectToHost(QString::fromStdString(serverFromPoolForDisplay.host),serverFromPoolForDisplay.port);
    return returnVar;
}

void Api_protocol_Qt::send_player_direction(const CatchChallenger::Direction &the_direction)
{
    Api_protocol::send_player_direction(the_direction);
}

void Api_protocol_Qt::newError(const std::string &error,const std::string &detailedError)
{
    emit QtnewError(error,detailedError);
}
void Api_protocol_Qt::message(const std::string &message)
{
    emit Qtmessage(message);
}
void Api_protocol_Qt::lastReplyTime(const uint32_t &time)
{
    emit QtlastReplyTime(time);
}

//protocol/connection info
void Api_protocol_Qt::disconnected(const std::string &reason)
{
    emit Qtdisconnected(reason);
}
void Api_protocol_Qt::notLogged(const std::string &reason)
{
    emit QtnotLogged(reason);
}
void Api_protocol_Qt::logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList)
{
    emit Qtlogged(characterEntryList);
}
void Api_protocol_Qt::protocol_is_good()
{
    emit Qtprotocol_is_good();
}
void Api_protocol_Qt::connectedOnLoginServer()
{
    emit QtconnectedOnLoginServer();
}
void Api_protocol_Qt::connectingOnGameServer()
{
    emit QtconnectingOnGameServer();
}
void Api_protocol_Qt::connectedOnGameServer()
{
    emit QtconnectedOnGameServer();
}
void Api_protocol_Qt::haveDatapackMainSubCode()
{
    emit QthaveDatapackMainSubCode();
}
void Api_protocol_Qt::gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression)
{
    emit QtgatewayCacheUpdate(gateway,progression);
}

//general info
void Api_protocol_Qt::number_of_player(const uint16_t &number,const uint16_t &max_players)
{
    emit Qtnumber_of_player(number,max_players);
}
void Api_protocol_Qt::random_seeds(const std::string &data)
{
    emit Qtrandom_seeds(data);
}

//character
void Api_protocol_Qt::newCharacterId(const uint8_t &returnCode,const uint32_t &characterId)
{
    emit QtnewCharacterId(returnCode,characterId);
}
void Api_protocol_Qt::haveCharacter()
{
    emit QthaveCharacter();
}
//events
void Api_protocol_Qt::setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events)
{
    emit QtsetEvents(events);
}
void Api_protocol_Qt::newEvent(const uint8_t &event,const uint8_t &event_value)
{
    emit QtnewEvent(event,event_value);
}

//map move
void Api_protocol_Qt::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit Qtinsert_player(player,mapId,x,y,direction);
}
void Api_protocol_Qt::move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement)
{
    emit Qtmove_player(id,movement);
}
void Api_protocol_Qt::remove_player(const uint16_t &id)
{
    emit Qtremove_player(id);
}
void Api_protocol_Qt::reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit Qtreinsert_player(id,x,y,direction);
}
void Api_protocol_Qt::full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction)
{
    emit Qtfull_reinsert_player(id,mapId,x,y,direction);
}
void Api_protocol_Qt::dropAllPlayerOnTheMap()
{
    emit QtdropAllPlayerOnTheMap();
}
void Api_protocol_Qt::teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit QtteleportTo(mapId,x,y,direction);
}

//plant
void Api_protocol_Qt::insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature)
{
    emit Qtinsert_plant(mapId,x,y,plant_id,seconds_to_mature);
}
void Api_protocol_Qt::remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y)
{
    emit Qtremove_plant(mapId,x,y);
}
void Api_protocol_Qt::seed_planted(const bool &ok)
{
    emit Qtseed_planted(ok);
}
void Api_protocol_Qt::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    emit Qtplant_collected(stat);
}
//crafting
void Api_protocol_Qt::recipeUsed(const RecipeUsage &recipeUsage)
{
    emit QtrecipeUsed(recipeUsage);
}
//inventory
void Api_protocol_Qt::objectUsed(const ObjectUsage &objectUsage)
{
    emit QtobjectUsed(objectUsage);
}
void Api_protocol_Qt::monsterCatch(const bool &success)
{
    emit QtmonsterCatch(success);
}

//chat
void Api_protocol_Qt::new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type)
{
    emit Qtnew_chat_text(chat_type,text,pseudo,player_type);
}
void Api_protocol_Qt::new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text)
{
    emit Qtnew_system_text(chat_type,text);
}

//player info
void Api_protocol_Qt::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    emit Qthave_current_player_info(informations);
}
void Api_protocol_Qt::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items)
{
    emit Qthave_inventory(items,warehouse_items);
}
void Api_protocol_Qt::add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    emit Qtadd_to_inventory(items);
}
void Api_protocol_Qt::remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    emit Qtremove_to_inventory(items);
}

//datapack
void Api_protocol_Qt::haveTheDatapack()
{
    std::cerr << "Api_protocol_Qt::haveTheDatapack()" << std::endl;
    emit QthaveTheDatapack();
}
void Api_protocol_Qt::haveTheDatapackMainSub()
{
    emit QthaveTheDatapackMainSub();
}
//base
void Api_protocol_Qt::newFileBase(const std::string &fileName,const std::string &data)
{
    emit QtnewFileBase(fileName,data);
}
void Api_protocol_Qt::newHttpFileBase(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileBase(url,fileName);
}
void Api_protocol_Qt::removeFileBase(const std::string &fileName)
{
    emit QtremoveFileBase(fileName);
}
void Api_protocol_Qt::datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeBase(datapckFileNumber,datapckFileSize);
}
//main
void Api_protocol_Qt::newFileMain(const std::string &fileName,const std::string &data)
{
    emit QtnewFileMain(fileName,data);
}
void Api_protocol_Qt::newHttpFileMain(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileMain(url,fileName);
}
void Api_protocol_Qt::removeFileMain(const std::string &fileName)
{
    emit QtremoveFileMain(fileName);
}
void Api_protocol_Qt::datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeMain(datapckFileNumber,datapckFileSize);
}
//sub
void Api_protocol_Qt::newFileSub(const std::string &fileName,const std::string &data)
{
    emit QtnewFileSub(fileName,data);
}
void Api_protocol_Qt::newHttpFileSub(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileSub(url,fileName);
}
void Api_protocol_Qt::removeFileSub(const std::string &fileName)
{
    emit QtremoveFileSub(fileName);
}
void Api_protocol_Qt::datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeSub(datapckFileNumber,datapckFileSize);
}

//shop
void Api_protocol_Qt::haveShopList(const std::vector<ItemToSellOrBuy> &items)
{
    emit QthaveShopList(items);
}
void Api_protocol_Qt::haveBuyObject(const BuyStat &stat,const uint32_t &newPrice)
{
    emit QthaveBuyObject(stat,newPrice);
}
void Api_protocol_Qt::haveSellObject(const SoldStat &stat,const uint32_t &newPrice)
{
    emit QthaveSellObject(stat,newPrice);
}

//factory
void Api_protocol_Qt::haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products)
{
    emit QthaveFactoryList(remainingProductionTime,resources,products);
}
void Api_protocol_Qt::haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice)
{
    emit QthaveBuyFactoryObject(stat,newPrice);
}
void Api_protocol_Qt::haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice)
{
    emit QthaveSellFactoryObject(stat,newPrice);
}

//trade
void Api_protocol_Qt::tradeRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QttradeRequested(pseudo,skinInt);
}
void Api_protocol_Qt::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QttradeAcceptedByOther(pseudo,skinInt);
}
void Api_protocol_Qt::tradeCanceledByOther()
{
    emit QttradeCanceledByOther();
}
void Api_protocol_Qt::tradeFinishedByOther()
{
    emit QttradeFinishedByOther();
}
void Api_protocol_Qt::tradeValidatedByTheServer()
{
    emit QttradeValidatedByTheServer();
}
void Api_protocol_Qt::tradeAddTradeCash(const uint64_t &cash)
{
    emit QttradeAddTradeCash(cash);
}
void Api_protocol_Qt::tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity)
{
    emit QttradeAddTradeObject(item,quantity);
}
void Api_protocol_Qt::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    emit QttradeAddTradeMonster(monster);
}

//battle
void Api_protocol_Qt::battleRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QtbattleRequested(pseudo,skinInt);
}
void Api_protocol_Qt::battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    emit QtbattleAcceptedByOther(pseudo,skinId,stat,monsterPlace,publicPlayerMonster);
}
void Api_protocol_Qt::battleCanceledByOther()
{
    emit QtbattleCanceledByOther();
}
void Api_protocol_Qt::sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn)
{
    emit QtsendBattleReturn(attackReturn);
}

//clan
void Api_protocol_Qt::clanActionSuccess(const uint32_t &clanId)
{
    emit QtclanActionSuccess(clanId);
}
void Api_protocol_Qt::clanActionFailed()
{
    emit QtclanActionFailed();
}
void Api_protocol_Qt::clanDissolved()
{
    emit QtclanDissolved();
}
void Api_protocol_Qt::clanInformations(const std::string &name)
{
    emit QtclanInformations(name);
}
void Api_protocol_Qt::clanInvite(const uint32_t &clanId,const std::string &name)
{
    emit QtclanInvite(clanId,name);
}
void Api_protocol_Qt::cityCapture(const uint32_t &remainingTime,const uint8_t &type)
{
    emit QtcityCapture(remainingTime,type);
}

//city
void Api_protocol_Qt::captureCityYourAreNotLeader()
{
    emit QtcaptureCityYourAreNotLeader();
}
void Api_protocol_Qt::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone)
{
    emit QtcaptureCityYourLeaderHaveStartInOtherCity(zone);
}
void Api_protocol_Qt::captureCityPreviousNotFinished()
{
    emit QtcaptureCityPreviousNotFinished();
}
void Api_protocol_Qt::captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count)
{
    emit QtcaptureCityStartBattle(player_count,clan_count);
}
void Api_protocol_Qt::captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId)
{
    emit QtcaptureCityStartBotFight(player_count,clan_count,fightId);
}
void Api_protocol_Qt::captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count)
{
    emit QtcaptureCityDelayedStart(player_count,clan_count);
}
void Api_protocol_Qt::captureCityWin()
{
    emit QtcaptureCityWin();
}

//market
void Api_protocol_Qt::marketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,const std::vector<MarketMonster> &marketOwnMonsterList)
{
    emit QtmarketList(price,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
}
void Api_protocol_Qt::marketBuy(const bool &success)
{
    emit QtmarketBuy(success);
}
void Api_protocol_Qt::marketBuyMonster(const PlayerMonster &playerMonster)
{
    emit QtmarketBuyMonster(playerMonster);
}
void Api_protocol_Qt::marketPut(const bool &success)
{
    emit QtmarketPut(success);
}
void Api_protocol_Qt::marketGetCash(const uint64_t &cash)
{
    emit QtmarketGetCash(cash);
}
void Api_protocol_Qt::marketWithdrawCanceled()
{
    emit QtmarketWithdrawCanceled();
}
void Api_protocol_Qt::marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity)
{
    emit QtmarketWithdrawObject(objectId,quantity);
}
void Api_protocol_Qt::marketWithdrawMonster(const PlayerMonster &playerMonster)
{
    emit QtmarketWithdrawMonster(playerMonster);
}

std::string Api_protocol_Qt::getLanguage() const
{
    #ifndef CATCHCHALLENGER_BOT
        #ifndef BOTTESTCONNECT
        return Language::language.getLanguage().toStdString();
        #else
        return "en";
        #endif
    #else
    return "en";
    #endif
}

void Api_protocol_Qt::closeSocket()
{
    if(socket!=nullptr)
        socket->close();
}

ssize_t Api_protocol_Qt::read(char * data, const size_t &size)
{
    if(socket!=nullptr)
        return socket->read(data,size);
    abort();
    return -1;
}

ssize_t Api_protocol_Qt::write(const char * const data, const size_t &size)
{
    if(socket!=nullptr)
        return socket->write(data,size);
    abort();
    return -1;
}

void Api_protocol_Qt::error(const std::string &error)
{
    emit QtnewError(error,error);
}

void Api_protocol_Qt::setBattleMonster(const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(!battleCurrentMonster.empty() || !battleStat.empty() || !botFightMonsters.empty())
    {
        error("have already monster to set battle monster");
        return;
    }
    if(stat.empty())
    {
        error("monster list size can't be empty");
        return;
    }
    if(monsterPlace<=0 || monsterPlace>stat.size())
    {
        error("monsterPlace greater than monster list");
        return;
    }
    startTheFight();
    battleCurrentMonster.push_back(publicPlayerMonster);
    battleStat=stat;
    battleMonsterPlace.push_back(monsterPlace);
    mLastGivenXP=0;
}

void Api_protocol_Qt::setBotMonster(const std::vector<PlayerMonster> &botFightMonsters,const uint16_t &fightId)
{
    if(!battleCurrentMonster.empty() || !battleStat.empty() || !this->botFightMonsters.empty())
    {
        error("have already monster to set bot monster");
        return;
    }
    if(botFightMonsters.empty())
    {
        error("monster list size can't be empty");
        return;
    }
    if(this->fightId!=0)
    {
        error("Api_protocol_Qt::setBotMonster() fightId!=0");
        return;
    }
    startTheFight();
    this->fightId=fightId;
    this->botFightMonsters=botFightMonsters;
    unsigned int index=0;
    while(index<botFightMonsters.size())
    {
        botMonstersStat.push_back(0x01);
        index++;
    }
    mLastGivenXP=0;
}

bool Api_protocol_Qt::addBattleMonster(const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(battleStat.empty())
    {
        error("not monster stat list loaded");
        return false;
    }
    /*if(monsterPlace<=0 || monsterPlace>battleStat.size())
    {
        error("monsterPlace greater than monster list: "+std::to_string(monsterPlace));
        return false;
    }*/
    battleCurrentMonster.push_back(publicPlayerMonster);
    battleMonsterPlace.push_back(monsterPlace);
    return true;
}

bool Api_protocol_Qt::haveWin()
{
    if(!wildMonsters.empty())
    {
        if(wildMonsters.size()==1)
        {
            if(wildMonsters.front().hp==0)
            {
                emit message("remain one KO wild monsters");
                return true;
            }
            else
            {
                emit message("remain one wild monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 wild monsters").arg(wildMonsters.size()).toStdString());
            return false;
        }
    }
    if(!botFightMonsters.empty())
    {
        if(botFightMonsters.size()==1)
        {
            if(botFightMonsters.front().hp==0)
            {
                emit message("remain one KO botMonsters monsters");
                return true;
            }
            else
            {
                emit message("remain one botMonsters monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 bot monsters").arg(botFightMonsters.size()).toStdString());
            return false;
        }
    }
    if(!battleCurrentMonster.empty())
    {
        if(battleCurrentMonster.size()==1)
        {
            if(battleCurrentMonster.front().hp==0)
            {
                emit message("remain one KO battleCurrentMonster monsters");
                return true;
            }
            else
            {
                emit message("remain one battleCurrentMonster monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 battle monsters").arg(battleCurrentMonster.size()).toStdString());
            return false;
        }
    }
    emit message("no remaining monsters");
    return true;
}

bool Api_protocol_Qt::dropKOOtherMonster()
{
    bool commonReturn=CommonFightEngine::dropKOOtherMonster();

    bool battleReturn=false;
    if(!battleCurrentMonster.empty())
    {
        if(!battleStat.empty() && battleMonsterPlace.front()<battleStat.size())
        {
            battleCurrentMonster.erase(battleCurrentMonster.cbegin());
            battleStat[battleMonsterPlace.front()]=0x02;//not able to battle
            battleMonsterPlace.erase(battleMonsterPlace.cbegin());
        }
        battleReturn=true;
    }
    bool haveValidMonster=false;
    unsigned int index=0;
    while(index<battleStat.size())
    {
        if(battleStat.at(index)==0x01)
        {
            haveValidMonster=true;
            break;
        }
        index++;
    }
    if(!haveValidMonster)
        battleStat.clear();

    return commonReturn || battleReturn;
}

void Api_protocol_Qt::fightFinished()
{
    battleCurrentMonster.clear();
    battleStat.clear();
    battleMonsterPlace.clear();
    doTurnIfChangeOfMonster=true;
    CommonFightEngine::fightFinished();
}

bool Api_protocol_Qt::isInFight() const
{
    if(CommonFightEngine::isInFight())
        return true;
    else if(!battleStat.empty())
        return true;
    else
        return false;
}

std::vector<uint8_t> Api_protocol_Qt::addPlayerMonster(const PlayerMonster &playerMonster)
{
    std::vector<PlayerMonster> monsterList;
    monsterList.push_back(playerMonster);

    return addPlayerMonster(monsterList);
}

std::vector<uint8_t> Api_protocol_Qt::addPlayerMonster(const std::vector<PlayerMonster> &playerMonster)
{
    std::vector<uint8_t> positionList;
    const uint8_t basePosition=static_cast<uint8_t>(player_informations.playerMonster.size());
    Player_private_and_public_informations &informations=client->get_player_informations();
    unsigned int index=0;
    while(index<playerMonster.size())
    {
        const uint16_t &monsterId=playerMonster.at(index).monster;
        if(informations.encyclopedia_monster!=NULL)
            informations.encyclopedia_monster[monsterId/8]|=(1<<(7-monsterId%8));
        else
            std::cerr << "Api_protocol_Qt::addPlayerMonster(std::vector): encyclopedia_monster is null, unable to set" << std::endl;
        positionList.push_back(basePosition+static_cast<uint8_t>(index));
        index++;
    }
    CommonFightEngine::addPlayerMonster(playerMonster);
    return positionList;
}

bool Api_protocol_Qt::internalTryEscape()
{
    emit message("BaseWindow::on_toolButtonFightQuit_clicked(): emit tryEscape()");
    Api_protocol::tryEscape();
    return CommonFightEngine::internalTryEscape();
}

bool Api_protocol_Qt::tryCatchClient(const uint16_t &item)
{
    if(!playerMonster_catchInProgress.empty())
    {
        error("Añready try catch in progress");
        return false;
    }
    if(wildMonsters.empty())
    {
        error("Try catch when not with wild");
        return false;
    }
    emit message("Api_protocol_Qt::tryCapture(): emit tryCapture()");
    Api_protocol::useObject(item);
    PlayerMonster newMonster;
    newMonster.buffs=wildMonsters.front().buffs;
    newMonster.catched_with=item;
    newMonster.egg_step=0;
    newMonster.gender=wildMonsters.front().gender;
    newMonster.hp=wildMonsters.front().hp;
    #ifndef CATCHCHALLENGER_VERSION_SINGLESERVER
    newMonster.id=0;//unknown at this time
    #endif
    newMonster.level=wildMonsters.front().level;
    newMonster.monster=wildMonsters.front().monster;
    newMonster.remaining_xp=0;
    newMonster.skills=wildMonsters.front().skills;
    newMonster.sp=0;
    playerMonster_catchInProgress.push_back(newMonster);
    //need wait the server reply, monsterCatch(const bool &success)
    return true;
}

bool Api_protocol_Qt::catchInProgress() const
{
    return !playerMonster_catchInProgress.empty();
}

uint32_t Api_protocol_Qt::catchAWild(const bool &toStorage, const PlayerMonster &newMonster)
{
    Q_UNUSED(toStorage);
    Q_UNUSED(newMonster);
    return 0;
}

Skill::AttackReturn Api_protocol_Qt::doTheCurrentMonsterAttack(const uint16_t &skill,const uint8_t &skillLevel)
{
    fightEffectList.push_back(CommonFightEngine::doTheCurrentMonsterAttack(skill,skillLevel));
    return fightEffectList.back();
}

bool Api_protocol_Qt::applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn)
{
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
    {
        emit message("No current monster at apply current life effect");
        return false;
    }
    #ifdef DEBUG_CLIENT_BATTLE
    emit message("applyCurrentLifeEffectReturn on: "+QString::number(effectReturn.on));
    #endif
    int32_t quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
    switch(effectReturn.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        {

            PublicPlayerMonster *publicPlayerMonster;
            if(!wildMonsters.empty())
                publicPlayerMonster=&wildMonsters.front();
            else if(!botFightMonsters.empty())
                publicPlayerMonster=&botFightMonsters.front();
            else if(!battleCurrentMonster.empty())
                publicPlayerMonster=&battleCurrentMonster.front();
            else
            {
                emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"unknown other monster type");
                return false;
            }
            quantity=effectReturn.quantity;
            qDebug() << "applyCurrentLifeEffect() add hp on the ennemy " << quantity;
            if(quantity<0 && (-quantity)>(int32_t)publicPlayerMonster->hp)
            {
                qDebug() << "applyCurrentLifeEffect() ennemy is KO";
                publicPlayerMonster->hp=0;
                ableToFight=false;
            }
            else if(quantity>0 && quantity>(int32_t)(stat.hp-publicPlayerMonster->hp))
            {
                qDebug() << "applyCurrentLifeEffect() ennemy is fully healled";
                publicPlayerMonster->hp=stat.hp;
            }
            else
                publicPlayerMonster->hp+=quantity;
        }
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            quantity=effectReturn.quantity;
            qDebug() << "applyCurrentLifeEffect() add hp " << quantity;
            if(quantity<0 && (-quantity)>(int32_t)playerMonster->hp)
            {
                qDebug() << "applyCurrentLifeEffect() current monster is KO";
                playerMonster->hp=0;
            }
            else if(quantity>0 && quantity>(int32_t)(stat.hp-playerMonster->hp))
            {
                qDebug() << "applyCurrentLifeEffect() you are fully healled";
                playerMonster->hp=stat.hp;
            }
            else
                playerMonster->hp+=quantity;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
    return true;
}

void Api_protocol_Qt::addAndApplyAttackReturnList(const std::vector<Skill::AttackReturn> &attackReturnList)
{
    qDebug() << "addAndApplyAttackReturnList()";
    unsigned int index=0;
    unsigned int sub_index=0;
    while(index<attackReturnList.size())
    {
        const Skill::AttackReturn &attackReturn=attackReturnList.at(index);
        if(attackReturn.success)
        {
            sub_index=0;
            while(sub_index<attackReturn.addBuffEffectMonster.size())
            {
                Skill::BuffEffect buff=attackReturn.addBuffEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buff.on=invertApplyOn(buff.on);
                qDebug() << "addAndApplyAttackReturnList() buff on " << buff.on << ", buff:" << buff.buff << ", buff level:" << buff.level;
                addCurrentBuffEffect(buff);
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.removeBuffEffectMonster.size())
            {
                Skill::BuffEffect buff=attackReturn.removeBuffEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buff.on=invertApplyOn(buff.on);
                qDebug() << "addAndApplyAttackReturnList() buff on " << buff.on << ", buff:" << buff.buff << ", buff level:" << buff.level;
                removeBuffEffectFull(buff);
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.lifeEffectMonster.size())
            {
                Skill::LifeEffectReturn lifeEffect=attackReturn.lifeEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    lifeEffect.on=invertApplyOn(lifeEffect.on);
                qDebug() << "addAndApplyAttackReturnList() life effect on " << lifeEffect.on << ", quantity:" << lifeEffect.quantity;
                if(!applyCurrentLifeEffectReturn(lifeEffect))
                {
                    emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                                  "Error applying the life effect");
                    return;
                }
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.buffLifeEffectMonster.size())
            {
                Skill::LifeEffectReturn buffEffect=attackReturn.buffLifeEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buffEffect.on=invertApplyOn(buffEffect.on);
                qDebug() << "addAndApplyAttackReturnList() life effect on " << buffEffect.on << ", quantity:" << buffEffect.quantity;
                if(!applyCurrentLifeEffectReturn(buffEffect))
                {
                    emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                                  "Error applying the life effect");
                    return;
                }
                sub_index++;
            }
        }
        index++;
    }
    this->fightEffectList.insert(this->fightEffectList.cend(),attackReturnList.cbegin(),attackReturnList.cend());
}

PublicPlayerMonster * Api_protocol_Qt::getOtherMonster()
{
    if(!battleCurrentMonster.empty())
        return (PublicPlayerMonster *)&battleCurrentMonster.front();
    return CommonFightEngine::getOtherMonster();
}

uint8_t Api_protocol_Qt::getOtherSelectedMonsterNumber() const
{
    return 0;
}

std::vector<Skill::AttackReturn> Api_protocol_Qt::getAttackReturnList() const
{
    return fightEffectList;
}

Skill::AttackReturn Api_protocol_Qt::getFirstAttackReturn() const
{
    return fightEffectList.front();
}

Skill::AttackReturn Api_protocol_Qt::generateOtherAttack()
{
    fightEffectList.push_back(CommonFightEngine::generateOtherAttack());
    return fightEffectList.back();
}

void Api_protocol_Qt::removeTheFirstLifeEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().lifeEffectMonster.empty())
        fightEffectList.front().lifeEffectMonster.erase(fightEffectList.front().lifeEffectMonster.begin());
}

void Api_protocol_Qt::removeTheFirstBuffEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().buffLifeEffectMonster.empty())
        fightEffectList.front().buffLifeEffectMonster.erase(fightEffectList.front().buffLifeEffectMonster.begin());
}

void Api_protocol_Qt::removeTheFirstAddBuffEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().addBuffEffectMonster.empty())
        fightEffectList.front().addBuffEffectMonster.erase(fightEffectList.front().addBuffEffectMonster.begin());
}

void Api_protocol_Qt::removeTheFirstRemoveBuffEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().removeBuffEffectMonster.empty())
        fightEffectList.front().removeBuffEffectMonster.erase(fightEffectList.front().removeBuffEffectMonster.begin());
}

void Api_protocol_Qt::removeTheFirstAttackReturn()
{
    if(!fightEffectList.empty())
        fightEffectList.erase(fightEffectList.cbegin());
}

bool Api_protocol_Qt::firstAttackReturnHaveMoreEffect()
{
    if(fightEffectList.empty())
        return false;
    if(!fightEffectList.front().lifeEffectMonster.empty())
        return true;
    if(!fightEffectList.front().addBuffEffectMonster.empty())
        return true;
    if(!fightEffectList.front().removeBuffEffectMonster.empty())
        return true;
    if(!fightEffectList.front().buffLifeEffectMonster.empty())
        return true;
    return false;
}

bool Api_protocol_Qt::firstLifeEffectQuantityChange(int32_t quantity)
{
    if(fightEffectList.empty())
    {
        emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"try add quantity to non existant life effect");
        return false;
    }
    if(fightEffectList.front().lifeEffectMonster.empty())
    {
        emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"try add quantity to life effect list empty");
        return false;
    }
    fightEffectList.front().lifeEffectMonster.front().quantity+=quantity;
    return true;
}

void Api_protocol_Qt::setVariableContent(Player_private_and_public_informations player_informations_local)
{
    this->player_informations_local=player_informations_local;
    updateCanDoFight();
}

bool Api_protocol_Qt::isInBattle() const
{
    return !battleStat.empty();
}

bool Api_protocol_Qt::haveBattleOtherMonster() const
{
    return !battleCurrentMonster.empty();
}

bool Api_protocol_Qt::useSkill(const uint16_t &skill)
{
    const bool wasInBotFight=!botFightMonsters.empty();
    mLastGivenXP=0;
    Api_protocol::useSkill(skill);
    if(!isInBattle())
    {
        if(!CommonFightEngine::useSkill(skill))
            return false;
        return finishTheTurn(wasInBotFight);
    }
    else
        return true;
    ////> drop model server bool Client::useSkill(const uint16_t &skill)
}

bool Api_protocol_Qt::finishTheTurn(const bool &isBot)
{
    const bool &win=!currentMonsterIsKO() && otherMonsterIsKO();
    if(!isInFight())
    {
        if(win)
        {
            if(isBot)
            {
                if(player_informations.bot_already_beaten!=NULL)
                {
                    player_informations.bot_already_beaten[fightId/8]|=(1<<(7-fightId%8));
                    fightId=0;
                }
                else
                {
                    std::cerr << "Api_protocol_Qt::finishTheTurn() player_informations.bot_already_beaten==NULL: "+std::to_string(fightId) << std::endl;
                    abort();
                }
                std::cout << player_informations.public_informations.pseudo <<
                             ": Register the win against the bot fight: "+std::to_string(fightId) << std::endl;
            }
        }
    }
    return win;
}

void Api_protocol_Qt::catchIsDone()
{
    wildMonsters.erase(wildMonsters.begin());
}

bool Api_protocol_Qt::doTheOtherMonsterTurn()
{
    if(!isInBattle())
        return CommonFightEngine::doTheOtherMonsterTurn();
    return true;
}

void Api_protocol_Qt::levelUp(const uint8_t &level, const uint8_t &monsterIndex)//call after done the level
{
    CommonFightEngine::levelUp(level,monsterIndex);
    const PlayerMonster &monster=player_informations.playerMonster.at(monsterIndex);
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(monster.monster);
    unsigned int index=0;
    while(index<monsterInformations.evolutions.size())
    {
        const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
        if(evolution.type==Monster::EvolutionType_Level && evolution.data.level==level)//the current monster is not updated
        {
            mEvolutionByLevelUp.push_back(monsterIndex);
            return;
        }
        index++;
    }
}

PlayerMonster * Api_protocol_Qt::evolutionByLevelUp()
{
    if(mEvolutionByLevelUp.empty())
        return NULL;
    uint8_t monsterIndex=mEvolutionByLevelUp.front();
    mEvolutionByLevelUp.erase(mEvolutionByLevelUp.cbegin());
    return &player_informations.playerMonster[monsterIndex];
}

uint8_t Api_protocol_Qt::getPlayerMonsterPosition(const PlayerMonster * const playerMonster)
{
    unsigned int index=0;
    while(index<player_informations.playerMonster.size())
    {
        if(&player_informations.playerMonster.at(index)==playerMonster)
            return static_cast<uint8_t>(index);
        index++;
    }
    std::cerr << "getPlayerMonsterPosition() with not existing monster" << std::endl;
    return 0;
}

void Api_protocol_Qt::addToEncyclopedia(const uint16_t &monster)
{
    Player_private_and_public_informations &informations=Api_protocol::get_player_informations();
    if(informations.encyclopedia_monster!=NULL)
        informations.encyclopedia_monster[monster/8]|=(1<<(7-monster%8));
    else
        std::cerr << "Api_protocol_Qt::addPlayerMonster(PlayerMonster): encyclopedia_monster is null, unable to set" << std::endl;
}

void Api_protocol_Qt::confirmEvolutionByPosition(const uint8_t &monterPosition)
{
    Api_protocol::confirmEvolutionByPosition(monterPosition);
    CatchChallenger::PlayerMonster &playerMonster=player_informations.playerMonster[monterPosition];
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters[playerMonster.monster];
    unsigned int sub_index=0;
    while(sub_index<monsterInformations.evolutions.size())
    {
        if(monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Level)
        {
            playerMonster.monster=monsterInformations.evolutions.at(sub_index).evolveTo;
            addToEncyclopedia(playerMonster.monster);
            Monster::Stat stat=getStat(monsterInformations,playerMonster.level);
            if(playerMonster.hp>stat.hp)
                playerMonster.hp=stat.hp;
            return;
        }
        sub_index++;
    }
    qDebug() << "Evolution not found";
    return;
}

//return true if change level, multiplicator do at datapack loading
bool Api_protocol_Qt::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=CommonFightEngine::giveXPSP(xp,sp);
    mLastGivenXP=xp;
    return haveChangeOfLevel;
}

uint32_t Api_protocol_Qt::lastGivenXP()
{
    uint32_t tempLastGivenXP=mLastGivenXP;
    mLastGivenXP=0;
    return tempLastGivenXP;
}

void Api_protocol_Qt::errorFightEngine(const std::string &errorMessage)
{
    std::cerr << errorMessage << std::endl;
    error(errorMessage);
}

void Api_protocol_Qt::messageFightEngine(const std::string &message) const
{
    std::cout << message << std::endl;
}

uint32_t Api_protocol_Qt::randomSeedsSize() const
{
    return randomSeeds.size();
}

uint8_t Api_protocol_Qt::getOneSeed(const uint8_t &max)
{
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    std::cout << "Random seed remaining: " << randomSeeds.size() << std::endl;
    #endif
    const uint8_t &number=randomSeeds.at(0);
    randomSeeds.erase(randomSeeds.begin());
    return number%(max+1);
}

void Api_protocol_Qt::newRandomNumber(const std::string &data)
{
    randomSeeds.append(data);
}

void Api_protocol_Qt::setClient(Api_protocol_Qt * client)
{
    this->client=client;
}

//duplicate to have a return
bool Api_protocol_Qt::useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition)
{
    PlayerMonster * playerMonster=monsterByPosition(monsterPosition);
    if(playerMonster==NULL)
    {
        std::cerr << "Unable to locate the monster to use the item: " << std::to_string(monsterPosition) << std::endl;
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(object)!=CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend())
    {
    }
    //duplicate to have a return
    else if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend()
            ||
            CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend())
    {
        if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
        {
            //duplicate to have a return
            Skill::AttackReturn attackReturn;
            attackReturn.doByTheCurrentMonster=true;
            attackReturn.attackReturnCase=Skill::AttackReturnCase::AttackReturnCase_ItemUsage;
            //normal attack
            attackReturn.success=true;
            attackReturn.attack=0;
            //change monster if monsterPlace !=0
            attackReturn.monsterPlace=0;
            //use objet on monster if item!=0
            attackReturn.on_current_monster=true;
            attackReturn.item=object;

            const Monster::Stat &playerMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
            const std::vector<MonsterItemEffect> monsterItemEffect = CommonDatapack::commonDatapack.items.monsterItemEffect.at(object);
            unsigned int index=0;
            //duplicate to have a return
            while(index<monsterItemEffect.size())
            {
                const MonsterItemEffect &effect=monsterItemEffect.at(index);
                switch(effect.type)
                {
                    //duplicate to have a return
                    case MonsterItemEffectType_AddHp:
                        if(effect.data.hp>0 && (playerMonsterStat.hp-playerMonster->hp)>(uint32_t)effect.data.hp)
                        {
                            hpChange(playerMonster,playerMonster->hp+effect.data.hp);
                            Skill::LifeEffectReturn lifeEffectReturn;
                            lifeEffectReturn.quantity=effect.data.hp;
                            lifeEffectReturn.on=ApplyOn_Themself;
                            lifeEffectReturn.critical=false;
                            lifeEffectReturn.effective=1;
                            attackReturn.lifeEffectMonster.push_back(lifeEffectReturn);
                        }
                        //duplicate to have a return
                        else if(playerMonsterStat.hp!=playerMonster->hp)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            lifeEffectReturn.quantity=playerMonsterStat.hp-playerMonster->hp;
                            hpChange(playerMonster,playerMonsterStat.hp);
                            lifeEffectReturn.on=ApplyOn_Themself;
                            lifeEffectReturn.critical=false;
                            lifeEffectReturn.effective=1;
                            attackReturn.lifeEffectMonster.push_back(lifeEffectReturn);
                        }
                        else if(monsterItemEffect.size()==1)
                            return false;
                    break;
                    //duplicate to have a return
                    case MonsterItemEffectType_RemoveBuff:
                        if(effect.data.buff>0)
                        {
                            if(removeBuffOnMonster(playerMonster,effect.data.buff))
                            {
                                Skill::BuffEffect buffEffect;
                                buffEffect.buff=effect.data.buff;
                                buffEffect.on=ApplyOn_Themself;
                                buffEffect.level=1;
                                attackReturn.removeBuffEffectMonster.push_back(buffEffect);
                            }
                        }
                        else
                        {
                            if(!playerMonster->buffs.empty())
                            {
                                unsigned int index=0;
                                while(index<playerMonster->buffs.size())
                                {
                                    const PlayerBuff &playerBuff=playerMonster->buffs.at(index);
                                    Skill::BuffEffect buffEffect;
                                    buffEffect.buff=playerBuff.buff;
                                    buffEffect.on=ApplyOn_Themself;
                                    buffEffect.level=playerBuff.level;
                                    attackReturn.removeBuffEffectMonster.push_back(buffEffect);
                                    index++;
                                }
                            }
                            removeAllBuffOnMonster(playerMonster);
                        }
                    break;
                    default:
                        messageFightEngine(QStringLiteral("Item %1 have unknown effect").arg(object).toStdString());
                    break;
                }
                index++;
            }
            //duplicate to have a return
            if(isInFight())
            {
                doTheOtherMonsterTurn();
                this->fightEffectList.push_back(attackReturn);
            }
            return true;
        }
        else if(CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend())
        {
        }
    }
    else if(CommonDatapack::commonDatapack.items.itemToLearn.find(object)!=CommonDatapack::commonDatapack.items.itemToLearn.cend())
    {
    }
    else
        return false;

    return CommonFightEngine::useObjectOnMonsterByPosition(object,monsterPosition);
}

