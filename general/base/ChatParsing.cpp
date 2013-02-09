#include "ChatParsing.h"

using namespace CatchChallenger;

QString ChatParsing::new_chat_message(QString pseudo,Player_type player_type,Chat_type chat_type,QString text)
{
    QString returned_html;
    returned_html+="<div style=\"";
    switch(chat_type)
    {
        case Chat_type_local://local
        break;
        case Chat_type_all://all
        break;
        case Chat_type_clan://clan
            returned_html+="color:#FFBF00;";
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
        returned_html+=QString("%1").arg(text);
    else
        returned_html+=QString("%1: %2").arg(pseudo).arg(toSmilies(toHtmlEntities(text)));
    returned_html+="</div>";
    return returned_html;
}

QString ChatParsing::toHtmlEntities(QString text)
{
    text.replace("&","&amp;");
    text.replace("\"","&quot;");
    text.replace("'","&#039;");
    text.replace("<","&lt;");
    text.replace(">","&gt;");
    return text;
}

QString ChatParsing::toSmilies(QString text)
{
    text.replace(":)","<img src=\":/images/smiles/face-smile.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":|","<img src=\":/images/smiles/face-plain.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":(","<img src=\":/images/smiles/face-sad.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":P","<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":p","<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":D","<img src=\":/images/smiles/face-laugh.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":o","<img src=\":/images/smiles/face-surprise.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(";)","<img src=\":/images/smiles/face-wink.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    return text;
}
