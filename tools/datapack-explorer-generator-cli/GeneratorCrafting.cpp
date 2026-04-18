#include "GeneratorCrafting.hpp"
#include "GeneratorItems.hpp"
#include "Helper.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"

#include <sstream>

namespace GeneratorCrafting {

std::string relativePathForCraftingRecipe(uint16_t /*id*/)
{
    return "crafting.html";
}

static std::string itemImageUrl(const std::string &imagePath)
{
    if(imagePath.empty())
        return std::string();
    std::string rel=Helper::relativeFromDatapack(imagePath);
    std::string published=Helper::publishDatapackFile(rel);
    return Helper::relUrl(published);
}

void generate()
{
    Helper::setCurrentPage("crafting.html");

    std::ostringstream body;
    body << "<table class=\"item_list item_list_type_normal\">\n"
         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
         << "\t<th colspan=\"2\">Item</th>\n"
         << "<th>Material</th>\n"
         << "<th>Product</th>\n"
         << "\t<th>Price</th>\n"
         << "</tr>\n";

    for(uint16_t recipeId=1;recipeId<=CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipes_size()+1;recipeId++)
    {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_craftingRecipe(recipeId))
            continue;
        const CatchChallenger::CraftingRecipe &r=CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipe(recipeId);

        if(QtDatapackClientLoader::datapackLoader->has_itemExtra(r.itemToLearn))
        {
            const DatapackClientLoader::ItemExtra &learnExtra=QtDatapackClientLoader::datapackLoader->get_itemExtra(r.itemToLearn);
            std::string name=learnExtra.name;
            std::string link=Helper::relUrl(GeneratorItems::relativePathForItem(r.itemToLearn));
            std::string image=itemImageUrl(learnExtra.imagePath);

            body << "<tr class=\"value\">\n\t\t<td>\n";

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

            body << "</td>\n\t\t<td>\n";

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

            // Materials column
            body << "<td>\n";
            for(const CatchChallenger::CraftingRecipe::Material &mat : r.materials)
            {
                if(!QtDatapackClientLoader::datapackLoader->has_itemExtra(mat.item))
                    continue;
                const DatapackClientLoader::ItemExtra &matExtra=QtDatapackClientLoader::datapackLoader->get_itemExtra(mat.item);
                std::string matLink=Helper::relUrl(GeneratorItems::relativePathForItem(mat.item));
                std::string matImage=itemImageUrl(matExtra.imagePath);

                body << "<a href=\"" << Helper::htmlEscape(matLink) << "\" title=\""
                     << Helper::htmlEscape(matExtra.name) << "\">\n";
                body << "<table><tr><td>\n";
                if(!matImage.empty() && Helper::fileExists(Helper::relativeFromDatapack(matExtra.imagePath).empty() ? "" : Helper::datapackPath()+Helper::relativeFromDatapack(matExtra.imagePath)))
                    body << "<img src=\"" << matImage << "\" width=\"24\" height=\"24\" alt=\""
                         << Helper::htmlEscape(matExtra.name) << "\" title=\"" << Helper::htmlEscape(matExtra.name) << "\" />\n";
                body << "</td><td>\n";
                if(mat.quantity>1)
                    body << mat.quantity << "x ";
                body << matExtra.name << "</td></tr></table>\n";
                body << "</a>\n";
            }
            body << "</td>\n";

            // Product column
            body << "<td>\n";
            if(QtDatapackClientLoader::datapackLoader->has_itemExtra(r.doItemId))
            {
                const DatapackClientLoader::ItemExtra &prodExtra=QtDatapackClientLoader::datapackLoader->get_itemExtra(r.doItemId);
                std::string prodLink=Helper::relUrl(GeneratorItems::relativePathForItem(r.doItemId));
                std::string prodImage=itemImageUrl(prodExtra.imagePath);

                body << "<a href=\"" << Helper::htmlEscape(prodLink) << "\" title=\""
                     << Helper::htmlEscape(prodExtra.name) << "\">\n";
                body << "<table><tr><td>\n";
                if(!prodImage.empty() && Helper::fileExists(Helper::datapackPath()+Helper::relativeFromDatapack(prodExtra.imagePath)))
                    body << "<img src=\"" << prodImage << "\" width=\"24\" height=\"24\" alt=\""
                         << Helper::htmlEscape(prodExtra.name) << "\" title=\"" << Helper::htmlEscape(prodExtra.name) << "\" />\n";
                body << "</td><td>" << prodExtra.name << "</td></tr></table>\n";
                body << "</a>\n";
            }
            body << "</td>\n";

            // Price column
            uint32_t price=0;
            if(CatchChallenger::CommonDatapack::commonDatapack.has_item(r.itemToLearn))
                price=CatchChallenger::CommonDatapack::commonDatapack.get_item(r.itemToLearn).price;
            body << "<td>" << price << "$</td>\n";

            body << "</tr>\n";
        }
        else
            body << "<tr class=\"value\"><td colspan=\"3\">Item to learn missing: " << r.itemToLearn << "</td></tr>\n";
    }

    body << "<tr>\n\t<td colspan=\"5\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";

    Helper::writeHtml("crafting.html","Crafting list",body.str());
}

} // namespace GeneratorCrafting
