#include "ChatParsing.h"

using namespace CatchChallenger;

std::string ChatParsing::new_chat_message(const std::string &pseudo, const Player_type &player_type, const Chat_type &chat_type, const std::string &text)
{
    std::string returned_html;
    returned_html+="<div style=\"";
    switch(chat_type)
    {
        case Chat_type_local://local
            returned_html+="font-style:italic;";
        break;
        case Chat_type_all://all
        break;
        case Chat_type_clan://clan
            returned_html+="color:#AA7F00;";
        break;
        case Chat_type_aliance://aliance
            returned_html+="color:#60BF20;";
        break;
        case Chat_type_pm://to player
            returned_html+="color:#5C255F;";
        break;
        case Chat_type_system://system
            returned_html+="color:#707070;font-style:italic;font-size:small;";
        break;
        case Chat_type_system_important://system importance: hight
            returned_html+="color:#60BF20;";
        break;
        default:
        break;
    }
    switch(player_type)
    {
        case Player_type_normal://normal player
        break;
        case Player_type_premium://premium player
        break;
        case Player_type_gm://gm
            returned_html+="font-weight:bold;";
        break;
        case Player_type_dev://dev
            returned_html+="font-weight:bold;";
        break;
        default:
        break;
    }
    returned_html+="\">";
    switch(player_type)
    {
        case Player_type_normal://normal player
        break;
        case Player_type_premium://premium player
            returned_html+="<img src=\":/images/chat/premium.png\" alt\"\" />";
        break;
        case Player_type_gm://gm
            returned_html+="<img src=\":/images/chat/admin.png\" alt\"\" />";
        break;
        case Player_type_dev://dev
            returned_html+="<img src=\":/images/chat/developer.png\" alt\"\" />";
        break;
        default:
        break;
    }
    if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
        returned_html+=text;
    else
    {
        if(pseudo.empty())
            returned_html+=toSmilies(toHtmlEntities(text));
        else
            returned_html+=pseudo+": "+toSmilies(toHtmlEntities(text));
    }
    returned_html+="</div>";
    return returned_html;
}

std::string ChatParsing::toHtmlEntities(const std::string &text)
{
    std::string newstring=text;
    stringreplaceAll(newstring,"&","&amp;");
    stringreplaceAll(newstring,"\"","&quot;");
    stringreplaceAll(newstring,"'","&#039;");
    stringreplaceAll(newstring,"<","&lt;");
    stringreplaceAll(newstring,">","&gt;");
    return newstring;
}

std::string ChatParsing::toSmilies(const std::string &text)
{
    std::string newstring=text;
    stringreplaceAll(newstring,":)","<img src=\":/images/smiles/face-smile.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    stringreplaceAll(newstring,":|","<img src=\":/images/smiles/face-plain.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    stringreplaceAll(newstring,":(","<img src=\":/images/smiles/face-sad.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    stringreplaceAll(newstring,":P","<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    stringreplaceAll(newstring,":p","<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    stringreplaceAll(newstring,":D","<img src=\":/images/smiles/face-laugh.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    stringreplaceAll(newstring,":o","<img src=\":/images/smiles/face-surprise.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    stringreplaceAll(newstring,";)","<img src=\":/images/smiles/face-wink.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    return newstring;
}
