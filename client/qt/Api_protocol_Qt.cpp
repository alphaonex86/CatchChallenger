#include "Api_protocol_Qt.h"
#include "LanguagesSelect.h"
#ifndef NOTCPSOCKET
#include "SslCert.h"
#include <QSslKey>
#endif
#include <iostream>
#include <cstring>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

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
                    SslCert sslCert(NULL);
                    sslCert.exec();
                    if(sslCert.validated())
                        saveCert(certFile.fileName().toStdString());
                    else
                    {
                        socket->sslSocket->disconnectFromHost();
                        return;
                    }
                    #endif
                }
                else if(certFile.open(QIODevice::ReadOnly))
                {
                    if(socket->sslSocket->peerCertificate().publicKey().toPem()!=certFile.readAll())
                    {
                        #if (!defined(CATCHCHALLENGER_VERSION_SOLO) || defined(CATCHCHALLENGER_MULTI)) && ! defined(BOTTESTCONNECT)
                        SslCert sslCert(NULL);
                        sslCert.exec();
                        if(sslCert.validated())
                            saveCert(certFile.fileName().toStdString());
                        else
                        {
                            socket->sslSocket->disconnectFromHost();
                            return;
                        }
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
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::Organization);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::CommonName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::LocalityName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::OrganizationalUnitName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::CountryName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::StateOrProvinceName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::EmailAddress);
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
    #ifndef BOTTESTCONNECT
    if(LanguagesSelect::languagesSelect==NULL)
        return "en";
    else
        return LanguagesSelect::languagesSelect->getCurrentLanguages();
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
