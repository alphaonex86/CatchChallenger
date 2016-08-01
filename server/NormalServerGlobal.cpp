#include "NormalServerGlobal.h"
#include "VariableServer.h"
#include "base/Client.h"
#include "../general/base/FacilityLibGeneral.h"

#include <regex>

#include <random>
#include <iostream>

NormalServerGlobal::NormalServerGlobal()
{
}

void NormalServerGlobal::displayInfo()
{
    #if defined(__clang__)
        std::cout << "Llvm and clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__ << ", version string: " << __clang_version__ << " build: " << __DATE__ << " " << __TIME__ << std::endl;
    #else
        #if defined(__GNUC__)
            std::cout << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << " build: " << __DATE__ << " " << __TIME__ << std::endl;
        #else
            #if defined(__DATE__) && defined(__TIME__)
                std::cout << "Unknown compiler: " << __DATE__ << " " << __TIME__ << std::endl;
            #else
                std::cout << "Unknown compiler" << std::endl;
            #endif
        #endif
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::cout << "Qt version: " << qVersion() << " (" << QT_VERSION << ")" << std::endl;
    #endif
    std::cout << "Base client size without string/pointer content: " << sizeof(CatchChallenger::Client)
              << ": ("
              << "Client: " << (sizeof(CatchChallenger::Client)-sizeof(BaseClassSwitch)-sizeof(CatchChallenger::ProtocolParsingInputOutput)-sizeof(CatchChallenger::CommonFightEngine)-sizeof(CatchChallenger::ClientMapManagement)) << " + "
              << "BaseClassSwitch: " << sizeof(BaseClassSwitch) << " + "
              << "ProtocolParsingInputOutput: " << sizeof(CatchChallenger::ProtocolParsingInputOutput) << " + "
              << "CommonFightEngine: " << sizeof(CatchChallenger::CommonFightEngine) << " ("
                << "CommonFightEngine: " << (sizeof(CatchChallenger::CommonFightEngine)-sizeof(CatchChallenger::ClientBase)-sizeof(CatchChallenger::CommonFightEngineBase)) << " + "
                    << "ClientBase: " << sizeof(CatchChallenger::ClientBase) << "("
                        << "ClientBase: " << (sizeof(CatchChallenger::ClientBase)-sizeof(CatchChallenger::Player_private_and_public_informations)) << " + "
                        << "Player_private_and_public_informations: " << sizeof(CatchChallenger::Player_private_and_public_informations) << " + "
                        << "CommonFightEngineBase: " << sizeof(CatchChallenger::CommonFightEngineBase)
                      << ") + "
                    << "CommonFightEngineBase: " << sizeof(CatchChallenger::CommonFightEngineBase)
                  << ") + "
              << "ClientMapManagement: " << sizeof(CatchChallenger::ClientMapManagement) << ")"
              << std::endl;
}

void NormalServerGlobal::checkSettingsFile(TinyXMLSettings * const settings, const std::string &datapack_basePath)
{
    if(!settings->contains("max-players"))
        settings->setValue("max-players",200);
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(!settings->contains("character_delete_time"))
        settings->setValue("character_delete_time",604800);
    if(!settings->contains("max_pseudo_size"))
        settings->setValue("max_pseudo_size",20);
    if(!settings->contains("max_character"))
        settings->setValue("max_character",3);
    if(!settings->contains("min_character"))
        settings->setValue("min_character",1);
    if(!settings->contains("automatic_account_creation"))
    #if defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
        settings->setValue("automatic_account_creation",false);
    #else
        settings->setValue("automatic_account_creation",true);
    #endif

    if(!settings->contains("maxPlayerMonsters"))
        settings->setValue("maxPlayerMonsters",8);
    if(!settings->contains("maxWarehousePlayerMonsters"))
        settings->setValue("maxWarehousePlayerMonsters",30);
    if(!settings->contains("maxPlayerItems"))
        settings->setValue("maxPlayerItems",30);
    if(!settings->contains("maxWarehousePlayerItems"))
        settings->setValue("maxWarehousePlayerItems",150);
    #endif
    if(!settings->contains("everyBodyIsRoot"))
        settings->setValue("everyBodyIsRoot",false);
    if(!settings->contains("teleportIfMapNotFoundOrOutOfMap"))
        settings->setValue("teleportIfMapNotFoundOrOutOfMap",true);
    if(!settings->contains("server-ip"))
        settings->setValue("server-ip","");
    if(!settings->contains("common_blobversion_datapack"))
        settings->setValue("common_blobversion_datapack",0);
    if(!settings->contains("server_blobversion_datapack"))
        settings->setValue("server_blobversion_datapack",0);
    if(!settings->contains("pvp"))
        settings->setValue("pvp",true);
    if(!settings->contains("useSP"))
        settings->setValue("useSP",true);
    if(!settings->contains("autoLearn"))
        settings->setValue("autoLearn",false);
    if(!settings->contains("server-port"))
        settings->setValue("server-port",10000+rand()%(65535-10000));
    if(!settings->contains("sendPlayerNumber"))
        settings->setValue("sendPlayerNumber",false);
    if(!settings->contains("tolerantMode"))
        settings->setValue("tolerantMode",false);
    if(!settings->contains("compression"))
        settings->setValue("compression","zlib");
    if(!settings->contains("compressionLevel"))
        settings->setValue("compressionLevel",6);
    if(!settings->contains("anonymous"))
        settings->setValue("anonymous",false);
    if(!settings->contains("server_message"))
        settings->setValue("server_message","");
    if(!settings->contains("proxy"))
        settings->setValue("proxy","");
    if(!settings->contains("proxy_port"))
        settings->setValue("proxy_port",9050);
    if(!settings->contains("forcedSpeed"))
        settings->setValue("forcedSpeed",CATCHCHALLENGER_SERVER_NORMAL_SPEED);
    if(!settings->contains("dontSendPseudo"))
        settings->setValue("dontSendPseudo",false);
    if(!settings->contains("dontSendPlayerType"))
        settings->setValue("dontSendPlayerType",false);
    if(!settings->contains("forceClientToSendAtMapChange"))
        settings->setValue("forceClientToSendAtMapChange",true);
    if(!settings->contains("httpDatapackMirror"))
        settings->setValue("httpDatapackMirror","");
    if(!settings->contains("datapackCache"))
        settings->setValue("datapackCache",-1);
    if(!settings->contains("plantOnlyVisibleByPlayer"))
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        settings->setValue("plantOnlyVisibleByPlayer",true);
    #else
        settings->setValue("plantOnlyVisibleByPlayer",false);
    #endif
    if(!settings->contains("useSsl"))
    #ifdef __linux__
        settings->setValue("useSsl",false);
    #else
        settings->setValue("useSsl",false);
    #endif
    if(!settings->contains("mainDatapackCode"))
    {
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &fileInfoList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(datapack_basePath+"map/main/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(fileInfoList.size()==1)
        {
            const std::string &string=fileInfoList.at(0).name;
            if(regex_search(string,std::regex("^[a-z0-9\\- _]+$",std::regex_constants::optimize)))
                settings->setValue("mainDatapackCode",string);
        }
        else
            settings->setValue("mainDatapackCode","[main]");
    }
    if(!settings->contains("subDatapackCode"))
        settings->setValue("subDatapackCode","");

    if(!settings->contains("exportedXml"))
        settings->setValue("exportedXml","");
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    settings->beginGroup("master");
    if(!settings->contains("external-server-ip"))
        settings->setValue("external-server-ip",settings->value("server-ip"));
    if(!settings->contains("external-server-port"))
        settings->setValue("external-server-port",settings->value("server-port"));
    {
        std::mt19937 rng;
        rng.seed(time(0));
        if(!settings->contains("uniqueKey"))
            settings->setValue("uniqueKey",std::to_string(static_cast<unsigned int>(rng())));
        if(!settings->contains("charactersGroup"))
            settings->setValue("charactersGroup","PutHereTheInfoLike-"+std::to_string(rng()));
    }
    if(!settings->contains("logicalGroup"))
        settings->setValue("logicalGroup","");

    if(!settings->contains("host"))
        settings->setValue("host","localhost");
    if(!settings->contains("port"))
        settings->setValue("port",9999);
    if(!settings->contains("considerDownAfterNumberOfTry"))
        settings->setValue("considerDownAfterNumberOfTry",3);
    if(!settings->contains("tryInterval"))
        settings->setValue("tryInterval",5);
    if(!settings->contains("token"))
        settings->setValue("token","");
    if(!settings->contains("maxLockAge"))
        settings->setValue("maxLockAge",10*60);
    if(!settings->contains("purgeLockPeriod"))
        settings->setValue("purgeLockPeriod",3*60);
    settings->endGroup();
    #endif

    #ifdef __linux__
    settings->beginGroup("Linux");
    if(!settings->contains("tcpCork"))
        settings->setValue("tcpCork",false);
    if(!settings->contains("tcpNodelay"))
        settings->setValue("tcpNodelay",false);
    settings->endGroup();
    #endif

    settings->beginGroup("MapVisibilityAlgorithm");
    if(!settings->contains("MapVisibilityAlgorithm"))
        settings->setValue("MapVisibilityAlgorithm",0);
    settings->endGroup();

    settings->beginGroup("DDOS");
    if(!settings->contains("waitBeforeConnectAfterKick"))
        settings->setValue("waitBeforeConnectAfterKick",30);
    if(!settings->contains("computeAverageValueTimeInterval"))
        settings->setValue("computeAverageValueTimeInterval",5);
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    if(!settings->contains("kickLimitMove"))
        settings->setValue("kickLimitMove",60);
    if(!settings->contains("kickLimitChat"))
        settings->setValue("kickLimitChat",5);
    if(!settings->contains("kickLimitOther"))
        settings->setValue("kickLimitOther",30);
    #endif
    if(!settings->contains("dropGlobalChatMessageGeneral"))
        settings->setValue("dropGlobalChatMessageGeneral",20);
    if(!settings->contains("dropGlobalChatMessageLocalClan"))
        settings->setValue("dropGlobalChatMessageLocalClan",20);
    if(!settings->contains("dropGlobalChatMessagePrivate"))
        settings->setValue("dropGlobalChatMessagePrivate",20);
    settings->endGroup();

    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    if(!settings->contains("Max"))
        settings->setValue("Max",50);
    if(!settings->contains("Reshow"))
        settings->setValue("Reshow",30);
    if(!settings->contains("Reemit"))
        settings->setValue("Reemit",false);
    settings->endGroup();

    settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
    if(!settings->contains("MaxWithBorder"))
        settings->setValue("MaxWithBorder",20);
    if(!settings->contains("ReshowWithBorder"))
        settings->setValue("ReshowWithBorder",10);
    if(!settings->contains("Max"))
        settings->setValue("Max",50);
    if(!settings->contains("Reshow"))
        settings->setValue("Reshow",30);
    settings->endGroup();

    settings->beginGroup("rates");
    if(!settings->contains("xp_normal"))
        settings->setValue("xp_normal",1.0);
    if(!settings->contains("gold_normal"))
        settings->setValue("gold_normal",1.0);
    if(!settings->contains("drop_normal"))
        settings->setValue("drop_normal",1.0);
    if(!settings->contains("xp_pow_normal"))
        settings->setValue("xp_pow_normal",1.0);
    settings->endGroup();

    settings->beginGroup("chat");
    if(!settings->contains("allow-all"))
        settings->setValue("allow-all",true);
    if(!settings->contains("allow-local"))
        settings->setValue("allow-local",true);
    if(!settings->contains("allow-private"))
        settings->setValue("allow-private",true);
    if(!settings->contains("allow-clan"))
        settings->setValue("allow-clan",true);
    settings->endGroup();

    if(!settings->contains("programmedEventFirstInit"))
    {
        settings->setValue("programmedEventFirstInit",true);
        settings->beginGroup("programmedEvent");
            settings->beginGroup("day");
                settings->beginGroup("day");
                if(!settings->contains("value"))
                    settings->setValue("value","day");
                if(!settings->contains("cycle"))
                    settings->setValue("cycle","60");
                if(!settings->contains("offset"))
                    settings->setValue("offset","0");
                settings->endGroup();
                settings->beginGroup("night");
                if(!settings->contains("value"))
                    settings->setValue("value","night");
                if(!settings->contains("cycle"))
                    settings->setValue("cycle","60");
                if(!settings->contains("offset"))
                    settings->setValue("offset","30");
                settings->endGroup();
            settings->endGroup();
        settings->endGroup();
    }

    settings->beginGroup("db");
    if(!settings->contains("db_fight_sync"))
        settings->setValue("db_fight_sync","FightSync_AtTheEndOfBattle");
    if(!settings->contains("secondToPositionSync"))
        settings->setValue("secondToPositionSync",0);
    if(!settings->contains("positionTeleportSync"))
        settings->setValue("positionTeleportSync",true);
    settings->endGroup();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    settings->beginGroup("db-login");
    if(!settings->contains("type"))
        settings->setValue("type",CATCHCHALLENGERDEFAULTDBTYPE);
    if(settings->value("type")=="sqlite")
    {
        if(!settings->contains("file"))
            settings->setValue("file","database.sqlite");
    }
    else
    {
        if(!settings->contains("host"))
            settings->setValue("host","localhost");
        if(!settings->contains("login"))
            settings->setValue("login","catchchallenger-login");
        if(!settings->contains("pass"))
            settings->setValue("pass","catchchallenger-pass");
        if(!settings->contains("db"))
            settings->setValue("db","catchchallenger_login");
    }
    if(!settings->contains("considerDownAfterNumberOfTry"))
        settings->setValue("considerDownAfterNumberOfTry",3);
    if(!settings->contains("tryInterval"))
        settings->setValue("tryInterval",5);
    settings->endGroup();

    settings->beginGroup("db-base");
    if(!settings->contains("type"))
        settings->setValue("type",CATCHCHALLENGERDEFAULTDBTYPE);
    if(settings->value("type")=="sqlite")
    {
        if(!settings->contains("file"))
            settings->setValue("file","database.sqlite");
    }
    else
    {
        if(!settings->contains("host"))
            settings->setValue("host","localhost");
        if(!settings->contains("login"))
            settings->setValue("login","catchchallenger-base");
        if(!settings->contains("pass"))
            settings->setValue("pass","catchchallenger-pass");
        if(!settings->contains("db"))
            settings->setValue("db","catchchallenger_base");
    }
    if(!settings->contains("considerDownAfterNumberOfTry"))
        settings->setValue("considerDownAfterNumberOfTry",3);
    if(!settings->contains("tryInterval"))
        settings->setValue("tryInterval",5);
    settings->endGroup();
    #endif

    settings->beginGroup("db-common");
    if(!settings->contains("type"))
        settings->setValue("type",CATCHCHALLENGERDEFAULTDBTYPE);
    if(settings->value("type")=="sqlite")
    {
        if(!settings->contains("file"))
            settings->setValue("file","database.sqlite");
    }
    else
    {
        if(!settings->contains("host"))
            settings->setValue("host","localhost");
        if(!settings->contains("login"))
            settings->setValue("login","catchchallenger-login");
        if(!settings->contains("pass"))
            settings->setValue("pass","catchchallenger-pass");
        if(!settings->contains("db"))
            settings->setValue("db","catchchallenger_common");
    }
    if(!settings->contains("considerDownAfterNumberOfTry"))
        settings->setValue("considerDownAfterNumberOfTry",3);
    if(!settings->contains("tryInterval"))
        settings->setValue("tryInterval",5);
    settings->endGroup();

    settings->beginGroup("db-server");
    if(!settings->contains("type"))
        settings->setValue("type",CATCHCHALLENGERDEFAULTDBTYPE);
    if(settings->value("type")=="sqlite")
    {
        if(!settings->contains("file"))
            settings->setValue("file","database.sqlite");
    }
    else
    {
        if(!settings->contains("host"))
            settings->setValue("host","localhost");
        if(!settings->contains("login"))
            settings->setValue("login","catchchallenger-login");
        if(!settings->contains("pass"))
            settings->setValue("pass","catchchallenger-pass");
        if(!settings->contains("db"))
            settings->setValue("db","catchchallenger_server");
    }
    if(!settings->contains("considerDownAfterNumberOfTry"))
        settings->setValue("considerDownAfterNumberOfTry",3);
    if(!settings->contains("tryInterval"))
        settings->setValue("tryInterval",5);
    settings->endGroup();


    settings->beginGroup("city");
    if(!settings->contains("capture_frequency"))
        settings->setValue("capture_frequency","month");
    if(!settings->contains("capture_day"))
        settings->setValue("capture_day","monday");
    if(!settings->contains("capture_time"))
        settings->setValue("capture_time","0:0");
    settings->endGroup();

    settings->sync();
}
