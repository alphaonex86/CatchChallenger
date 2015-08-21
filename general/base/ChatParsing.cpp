#include "ChatParsing.h"

using namespace CatchChallenger;

std::string ChatParsing::new_chat_message(std::string pseudo, Player_type player_type, Chat_type chat_type, std::string text)
{
    std::string returned_html;
    returned_html+=QLatin1Literal("<div style=\"");
    switch(chat_type)
    {
        case Chat_type_local://local
            returned_html+=QLatin1Literal("font-style:italic;");
        break;
        case Chat_type_all://all
        break;
        case Chat_type_clan://clan
            returned_html+=QLatin1Literal("color:#AA7F00;");
        break;
        case Chat_type_aliance://aliance
            returned_html+=QLatin1Literal("color:#60BF20;");
        break;
        case Chat_type_pm://to player
            returned_html+=QLatin1Literal("color:#5C255F;");
        break;
        case Chat_type_system://system
            returned_html+=QLatin1Literal("color:#707070;font-style:italic;font-size:small;");
        break;
        case Chat_type_system_important://system importance: hight
            returned_html+=QLatin1Literal("color:#60BF20;");
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
            returned_html+=QLatin1Literal("font-weight:bold;");
        break;
        case Player_type_dev://dev
            returned_html+=QLatin1Literal("font-weight:bold;");
        break;
        default:
        break;
    }
    returned_html+=QLatin1Literal("\">");
    switch(player_type)
    {
        case Player_type_normal://normal player
        break;
        case Player_type_premium://premium player
            returned_html+=QLatin1Literal("<img src=\":/images/chat/premium.png\" alt\"\" />");
        break;
        case Player_type_gm://gm
            returned_html+=QLatin1Literal("<img src=\":/images/chat/admin.png\" alt\"\" />");
        break;
        case Player_type_dev://dev
            returned_html+=QLatin1Literal("<img src=\":/images/chat/developer.png\" alt\"\" />");
        break;
        default:
        break;
    }
    if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
        returned_html+=text;
    else
    {
        if(pseudo.isEmpty())
            returned_html+=toSmilies(toHtmlEntities(text));
        else
            returned_html+=std::stringLiteral("%1: %2").arg(pseudo).arg(toSmilies(toHtmlEntities(text)));
    }
    returned_html+=QLatin1Literal("</div>");
    return returned_html;
}

std::string ChatParsing::toHtmlEntities(std::string text)
{
    text.replace(QLatin1Literal("&"),QLatin1Literal("&amp;"));
    text.replace(QLatin1Literal("\""),QLatin1Literal("&quot;"));
    text.replace(QLatin1Literal("'"),QLatin1Literal("&#039;"));
    text.replace(QLatin1Literal("<"),QLatin1Literal("&lt;"));
    text.replace(QLatin1Literal(">"),QLatin1Literal("&gt;"));
    return text;
}

std::string ChatParsing::toSmilies(std::string text)
{
    text.replace(QLatin1Literal(":)"),QLatin1Literal("<img src=\":/images/smiles/face-smile.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    text.replace(QLatin1Literal(":|"),QLatin1Literal("<img src=\":/images/smiles/face-plain.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    text.replace(QLatin1Literal(":("),QLatin1Literal("<img src=\":/images/smiles/face-sad.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    text.replace(QLatin1Literal(":P"),QLatin1Literal("<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    text.replace(QLatin1Literal(":p"),QLatin1Literal("<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    text.replace(QLatin1Literal(":D"),QLatin1Literal("<img src=\":/images/smiles/face-laugh.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    text.replace(QLatin1Literal(":o"),QLatin1Literal("<img src=\":/images/smiles/face-surprise.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    text.replace(QLatin1Literal(";)"),QLatin1Literal("<img src=\":/images/smiles/face-wink.png\" alt=\"\" style=\"vertical-align:middle;\" />"));
    return text;
}
