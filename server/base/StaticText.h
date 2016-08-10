#ifndef STATICTEXT_H
#define STATICTEXT_H

#include <string>
#include "GameServerVariables.h"

class StaticText
{
public:
    //player indexed list
    static const std::string text_chat;
    static const std::string text_space;
    static const std::string text_system;
    static const std::string text_system_important;
    static const std::string text_setrights;
    static const std::string text_normal;
    static const std::string text_premium;
    static const std::string text_gm;
    static const std::string text_dev;
    static const std::string text_playerlist;
    static const std::string text_startbold;
    static const std::string text_stopbold;
    static const std::string text_playernumber;
    static const std::string text_kick;
    static const std::string text_Youarealoneontheserver;
    static const std::string text_playersconnected;
    static const std::string text_playersconnectedspace;
    static const std::string text_havebeenkickedby;
    static const std::string text_unknowcommand;
    static const std::string text_commandnotunderstand;
    static const std::string text_command;
    static const std::string text_commaspace;
    static const std::string text_unabletofoundtheconnectedplayertokick;
    static const std::string text_unabletofoundthisrightslevel;
    #ifdef CATCHCHALLENGER_SERVER_DEBUG_COMMAND
    static const std::string text_debug;
    #endif

    static const std::string text_server_full;
    static const std::string text_slashpmspace;
    static const std::string text_slash;
    static const std::string text_send_command_slash;
    static const std::string text_trade;
    static const std::string text_battle;
    static const std::string text_give;
    static const std::string text_setevent;
    static const std::string text_take;
    static const std::string text_tp;
    static const std::string text_stop;
    static const std::string text_restart;
    static const std::string text_unknown_send_command_slash;
    static const std::string text_commands_seem_not_right;
    static const std::string single_quote;
    static const std::string antislash_single_quote;
    static const std::string text_dotslash;
    static const std::string text_dotcomma;
    static const std::string text_double_slash;
    static const std::string text_antislash;
    static const std::string text_top;
    static const std::string text_bottom;
    static const std::string text_left;
    static const std::string text_right;
    static const std::string text_dottmx;
    static const std::string text_unknown;
    static const std::string text_female;
    static const std::string text_male;
    static const std::string text_warehouse;
    static const std::string text_wear;
    static const std::string text_market;

    //used into Client::directionToStringToSave()
    static const std::string text_0;
    static const std::string text_1;
    static const std::string text_2;
    static const std::string text_3;
    static const std::string text_4;
    static const std::string text_false;
    static const std::string text_true;
    static const std::string text_to;

};

#endif // STATICTEXT_H
