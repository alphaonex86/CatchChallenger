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
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"skills/index.html")) << "\">Skills</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"types/index.html")) << "\">Types</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"buffs/index.html")) << "\">Buffs</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"plants/index.html")) << "\">Plants</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"crafting/index.html")) << "\">Crafting recipes</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"quests/index.html")) << "\">Quests</a></li>\n";
    body << "  <li><a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(page,"map/index.html")) << "\">Maps</a></li>\n";
    body << "</ul>\n";

    Helper::writeHtml(page,"Datapack explorer",body.str());
}

} // namespace GeneratorIndex
