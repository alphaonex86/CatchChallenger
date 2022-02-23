// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRY_HPP_
#define CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRY_HPP_

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../../../general/base/GeneralStructuresXml.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Combo.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/GridView.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ThemedButton.hpp"
#include "IndustryItem.hpp"

namespace Scenes {
class Industry : public QObject, public UI::Dialog {
    Q_OBJECT
   public:
    static Industry *Create();
    ~Industry();

    void OnEnter() override;
    void OnExit() override;
    void OnScreenResize() override;

    void SetBot(const CatchChallenger::Bot &bot, uint16_t industry_id);

   private:
    Sprite *npc_icon_;

    UI::Label *products_title_;
    UI::ListView *products_content_;
    UI::Button *products_buy_;

    UI::Label *resources_title_;
    UI::ListView *resources_content_;
    UI::Button *resources_sell_;

    Node *selected_product_;
    Node *selected_resource_;

    uint16_t industry_id_;
    CatchChallenger::IndustryStatus industryStatus;

    std::vector<CatchChallenger::ItemToSellOrBuy> items_sold_;
    std::vector<CatchChallenger::ItemToSellOrBuy> items_bought_;

    bool factoryInProduction;

    Industry();
    void OnProductClick(Node *node);
    void OnResourceClick(Node *node);
    void OnBuyClick();
    void OnSellClick();
    void UpdateContent();
    void TranslateLabels();

    void UpdateFactoryStatProduction(
        const CatchChallenger::IndustryStatus &industryStatus,
        const CatchChallenger::Industry &industry);
    void HaveFactoryListSlot(
        const uint32_t &remainingProductionTime,
        const std::vector<CatchChallenger::ItemToSellOrBuy> &resources,
        const std::vector<CatchChallenger::ItemToSellOrBuy> &products);
    void HaveBuyFactoryObjectSlot(const CatchChallenger::BuyStat &stat,
                                  const uint32_t &newPrice);
    void HaveSellFactoryObjectSlot(const CatchChallenger::SoldStat &stat,
                                   const uint32_t &newPrice);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRY_HPP_
