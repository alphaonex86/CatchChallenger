#include "GeneratorPlants.hpp"
#include "GeneratorItems.hpp"
#include "Helper.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"

#include <sstream>

namespace GeneratorPlants {

std::string relativePathForPlant(uint8_t id)
{
    return "plants/"+Helper::toStringUint(id)+".html";
}

static void writeTableHeader(std::ostringstream &body)
{
    body << "<table class=\"item_list item_list_type_normal plant_list\">\n"
         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
         << "\t<th colspan=\"2\">Plant</th>\n"
         << "\t<th colspan=\"2\">Time to grow</th>\n"
         << "\t<th>Fruits produced</th>\n"
         << "</tr>\n";
}

void generate()
{
    const auto &plants=CatchChallenger::CommonDatapack::commonDatapack.get_plants();
    const auto &itemsExtra=QtDatapackClientLoader::datapackLoader->get_itemsExtra();

    Helper::setCurrentPage("plants.html");

    std::ostringstream body;
    writeTableHeader(body);

    unsigned plant_count=0;
    for(const auto &p : plants)
    {
        const uint8_t id=p.first;
        const CatchChallenger::Plant &plant=p.second;

        plant_count++;
        if(plant_count>15)
        {
            body << "<tr>\n\t\t\t<td colspan=\"5\" class=\"item_list_endline item_list_title_type_normal\"></td>\n\t\t</tr>\n\t\t</table>\n";
            writeTableHeader(body);
            plant_count=1;
        }

        std::string name="???";
        std::string link;
        std::string image;

        auto ix=itemsExtra.find(plant.itemUsed);
        if(ix!=itemsExtra.cend())
        {
            name=ix->second.name;
            link=Helper::relUrl(GeneratorItems::relativePathForItem(plant.itemUsed));
            if(!ix->second.imagePath.empty())
            {
                std::string rel=Helper::relativeFromDatapack(ix->second.imagePath);
                if(Helper::fileExists(Helper::datapackPath()+rel))
                    image=Helper::relUrl(Helper::publishDatapackFile(rel));
            }
        }

        body << "<tr class=\"value\">\n\t<td>\n";

        // Item icon
        if(!image.empty())
        {
            if(!link.empty())
                body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
            body << "<img src=\"" << image << "\" width=\"24\" height=\"24\" alt=\""
                 << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
            if(!link.empty())
                body << "</a>\n";
        }

        body << "</td>\n\t<td>\n";

        // Item name
        if(!link.empty())
            body << "<a href=\"" << Helper::htmlEscape(link) << "\">\n";
        if(!name.empty())
            body << name;
        else
            body << "Unknown item";
        if(!link.empty())
            body << "</a>\n";
        body << "</td>\n";

        // Plant sprite
        body << "<td>\n";
        std::string spriteRel="plants/"+Helper::toStringUint(id)+".png";
        if(Helper::fileExists(Helper::datapackPath()+spriteRel))
        {
            std::string spriteUrl=Helper::relUrl(Helper::publishDatapackFile(spriteRel));
            body << "<img src=\"" << spriteUrl << "\" width=\"80\" height=\"32\" alt=\""
                 << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
        }
        body << "</td>\n";

        // Time to grow (fruits_seconds / 60 = minutes)
        body << "<td><b>" << (plant.fruits_seconds/60) << "</b> minutes</td>\n";

        // Fruits produced
        body << "<td>" << plant.fix_quantity << "</td>\n";

        body << "</tr>\n";
    }

    body << "<tr>\n\t<td colspan=\"5\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";

    Helper::writeHtml("plants.html","Plants list",body.str());
}

} // namespace GeneratorPlants
