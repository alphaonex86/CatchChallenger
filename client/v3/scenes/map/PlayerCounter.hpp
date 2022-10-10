// Copyright 2022 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_PLAYERCOUNTER_HPP_
#define CLIENT_V3_SCENES_MAP_PLAYERCOUNTER_HPP_

#include "../../core/Node.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Label.hpp"

namespace Scenes {
class PlayerCounter : public Node {
   public:
    PlayerCounter(Node* parent);
    ~PlayerCounter();
    static PlayerCounter* Create(Node *parent);

    void UpdateData(const uint16_t &number, const uint16_t &max_players);
    void Draw(QPainter *painter) override;

    void OnEnter() override;
    void OnExit() override;

   protected:
    Sprite *playersCountBack;
    UI::Label *playersCount;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_MAP_PLAYERCOUNTER_HPP_
