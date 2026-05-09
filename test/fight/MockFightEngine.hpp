#ifndef MOCKFIGHTENGINE_HPP
#define MOCKFIGHTENGINE_HPP

#include "../../general/fight/CommonFightEngine.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace CatchChallenger {

/// Minimal subclass of CommonFightEngine for unit testing.
///
/// Implements every pure virtual with in-memory state and exposes the
/// engine's protected members (wildMonsters, stepFight, ableToFight,
/// selectedMonster) so tests can inspect / drive flows that the real
/// server populates from network input.
///
/// Seeds: getOneSeed(max) returns seedQueue.front()%(max+1), or
/// fallback 0 when the queue is empty. Tests push deterministic seeds
/// to drive try-escape / try-capture / random-fight outcomes.
class MockFightEngine : public CommonFightEngine
{
public:
    MockFightEngine();
    ~MockFightEngine();

    // pure virtuals (CommonFightEngine.hpp)
    bool isInBattle() const override;
    uint8_t getOneSeed(const uint8_t &max) override;
    uint32_t randomSeedsSize() const override;
    uint32_t catchAWild(const bool &toStorage, const PlayerMonster &newMonster) override;
    Player_private_and_public_informations &get_public_and_private_informations() override;
    const Player_private_and_public_informations &get_public_and_private_informations_ro() const override;
    void errorFightEngine(const std::string &error) override;
    void messageFightEngine(const std::string &message) const override;

    // helpers exposing protected fight engine state
    void mockPushWildMonster(const PlayerMonster &m);
    void mockClearWild();
    void mockSetAbleToFight(bool v);
    void mockSetStepFight(uint8_t v);
    uint8_t mockStepFight() const;
    void mockResetCounters();
    void mockSetSelectedMonster(uint8_t v);

    // seed queue helpers
    void mockPushSeed(uint8_t s);
    void mockPushSeeds(const std::vector<uint8_t> &v);
    void mockClearSeeds();

    // capture / message recorders
    bool catchInvoked;
    PlayerMonster lastCaught;
    bool lastCaughtToStorage;
    std::vector<std::string> messages;
    std::vector<std::string> errors;

private:
    Player_private_and_public_informations player_;
    std::vector<uint8_t> seeds_;
    size_t seedIndex_;
};

} // namespace CatchChallenger

#endif
