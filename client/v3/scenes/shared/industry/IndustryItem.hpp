// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRYITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRYITEM_HPP_

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../core/Node.hpp"
#include "../../../ui/SelectableItem.hpp"

namespace Scenes {
class IndustryItem : public UI::SelectableItem {
   public:
    ~IndustryItem();
    static IndustryItem *Create(uint16_t object_id, uint32_t quantity,
                                uint32_t price);
    static IndustryItem *Create(CatchChallenger::ItemToSellOrBuy item);
    uint16_t ObjectId();
    uint32_t Quantity();
    uint32_t Price();
    void SetText(const QString &text);
    void SetIcon(const QPixmap &icon);
    void SetQuantity(uint32_t quantity);
    void SetPrice(uint32_t price);

    void DrawContent(QPainter *painter) override;
    void OnResize() override;

   private:
    uint16_t object_id_;
    uint32_t quantity_;
    uint32_t price_;

    QPixmap icon_;
    QString text_;

    IndustryItem(uint16_t object_id, uint32_t quantity, uint32_t price);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRYITEM_HPP_
