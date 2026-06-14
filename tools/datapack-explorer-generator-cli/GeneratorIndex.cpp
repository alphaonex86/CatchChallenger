#include "GeneratorIndex.hpp"
#include "Helper.hpp"

#include <sstream>

namespace GeneratorIndex {

void generate()
{
    const std::string page="index.html";
    Helper::setCurrentPage(page);
    std::ostringstream body;
    body << "<h1>Datapack explorer</h1>\n";
    body << "<ul class=\"list_links\">\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"items/index.html")) << "\">Items</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"monsters/index.html")) << "\">Monsters</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"skills.html")) << "\">Skills</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"types.html")) << "\">Types</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"buffs.html")) << "\">Buffs</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"plants.html")) << "\">Plants</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"crafting.html")) << "\">Crafting recipes</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"quests.html")) << "\">Quests</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"map/index.html")) << "\">Maps</a></li>\n";
    body << "</ul>\n";

    Helper::writeHtml(page,"Datapack explorer",body.str());

    // Legacy-URL stubs. The website's datapack-explorer.php landing page and
    // the shared template.html back_menu both link to top-level maps.html /
    // monsters.html / items.html (the layout the old PHP generator produced),
    // but this generator writes those lists as map/index.html,
    // monsters/index.html and items/index.html. Keep the old URLs alive with
    // instant meta-refresh redirects instead of rewriting every menu.
    const struct { const char *from; const char *to; } aliases[]={
        {"maps.html","map/index.html"},
        {"monsters.html","monsters/index.html"},
        {"items.html","items/index.html"},
    };
    for(const auto &a : aliases)
        Helper::writeFile(Helper::localPath()+a.from,
            std::string("<!DOCTYPE html>\n<html><head>"
                "<meta http-equiv=\"refresh\" content=\"0;url=")+a.to+"\">"
                "<link rel=\"canonical\" href=\""+a.to+"\">"
                "</head><body><a href=\""+a.to+"\">Moved</a></body></html>\n");
}

} // namespace GeneratorIndex
