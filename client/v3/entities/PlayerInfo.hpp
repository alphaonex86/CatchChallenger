// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_PLAYERINFO_HPP_
#define CLIENT_V3_ENTITIES_PLAYERINFO_HPP_

#include <QObject>
#include <functional>
#include <string>

#include "../../../general/base/GeneralStructures.hpp"
#include "../base/ConnectionManager.hpp"

class PlayerInfo : public QObject {
    Q_OBJECT
   public:
    static PlayerInfo* GetInstance();

    std::string ClanName;
    bool HaveClanInformation;

    ~PlayerInfo();
    CatchChallenger::Player_private_and_public_informations& GetInformation();
    const CatchChallenger::Player_private_and_public_informations&
    GetInformationRO() const;
    void Notify();
    void AddInventory(uint16_t item_id, uint32_t quantity);
    void AddInventory(const std::unordered_map<uint16_t, uint32_t>& items);
    void RemoveInventory(uint16_t item_id, uint32_t quantity);
    void AddCash(int32_t value);
    void UpdateMonsters(std::vector<CatchChallenger::PlayerMonster> monsters);
    bool IsAllowed(CatchChallenger::ActionAllow action);
    void AppendReputationPoints(const std::string& type,
                                const uint32_t& points);
    void AppendReputationPoints(
        const std::vector<CatchChallenger::ReputationRewards>& rewards);
    bool HaveReputationRequirements(
        const std::vector<CatchChallenger::ReputationRequirements>&
            reputations) const;
    void AppendReputationRewards(
        const std::vector<CatchChallenger::ReputationRewards>& reputations);

   signals:
    void OnUpdateInfo(PlayerInfo* info);
    void OnTipShowed(std::string tip, bool isOnlyText);

   private:
    static PlayerInfo* instance_;
    ConnectionManager* connection_manager_;

    PlayerInfo();
};
#endif  // CLIENT_V3_ENTITIES_PLAYERINFO_HPP_