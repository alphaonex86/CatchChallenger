#include "ClientFightEngine.h"
#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../base/Api_client_real.h"
#include "../../base/FacilityLibClient.h"
#include "../../../general/base/FacilityLib.h"

#include <QDebug>
#include <iostream>

using namespace CatchChallenger;

ClientFightEngine::ClientFightEngine()
{
    resetAll();
}

ClientFightEngine::~ClientFightEngine()
{
}

void ClientFightEngine::setBattleMonster(const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(!battleCurrentMonster.empty() || !battleStat.empty() || !botFightMonsters.empty())
    {
        emit error("have already monster to set battle monster");
        return;
    }
    if(stat.empty())
    {
        emit error("monster list size can't be empty");
        return;
    }
    if(monsterPlace<=0 || monsterPlace>stat.size())
    {
        emit error("monsterPlace greater than monster list");
        return;
    }
    startTheFight();
    battleCurrentMonster.push_back(publicPlayerMonster);
    battleStat=stat;
    battleMonsterPlace.push_back(monsterPlace);
    mLastGivenXP=0;
}

void ClientFightEngine::setBotMonster(const std::vector<PlayerMonster> &botFightMonsters,const uint16_t &fightId)
{
    if(!battleCurrentMonster.empty() || !battleStat.empty() || !this->botFightMonsters.empty())
    {
        emit error("have already monster to set bot monster");
        return;
    }
    if(botFightMonsters.empty())
    {
        emit error("monster list size can't be empty");
        return;
    }
    if(this->fightId!=0)
    {
        emit error("ClientFightEngine::setBotMonster() fightId!=0");
        return;
    }
    startTheFight();
    this->fightId=fightId;
    this->botFightMonsters=botFightMonsters;
    unsigned int index=0;
    while(index<botFightMonsters.size())
    {
        botMonstersStat.push_back(0x01);
        index++;
    }
    mLastGivenXP=0;
}

bool ClientFightEngine::addBattleMonster(const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(battleStat.empty())
    {
        emit error("not monster stat list loaded");
        return false;
    }
    /*if(monsterPlace<=0 || monsterPlace>battleStat.size())
    {
        emit error("monsterPlace greater than monster list: "+std::to_string(monsterPlace));
        return false;
    }*/
    battleCurrentMonster.push_back(publicPlayerMonster);
    battleMonsterPlace.push_back(monsterPlace);
    return true;
}

bool ClientFightEngine::haveWin()
{
    if(!wildMonsters.empty())
    {
        if(wildMonsters.size()==1)
        {
            if(wildMonsters.front().hp==0)
            {
                emit message("remain one KO wild monsters");
                return true;
            }
            else
            {
                emit message("remain one wild monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 wild monsters").arg(wildMonsters.size()).toStdString());
            return false;
        }
    }
    if(!botFightMonsters.empty())
    {
        if(botFightMonsters.size()==1)
        {
            if(botFightMonsters.front().hp==0)
            {
                emit message("remain one KO botMonsters monsters");
                return true;
            }
            else
            {
                emit message("remain one botMonsters monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 bot monsters").arg(botFightMonsters.size()).toStdString());
            return false;
        }
    }
    if(!battleCurrentMonster.empty())
    {
        if(battleCurrentMonster.size()==1)
        {
            if(battleCurrentMonster.front().hp==0)
            {
                emit message("remain one KO battleCurrentMonster monsters");
                return true;
            }
            else
            {
                emit message("remain one battleCurrentMonster monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 battle monsters").arg(battleCurrentMonster.size()).toStdString());
            return false;
        }
    }
    emit message("no remaining monsters");
    return true;
}

bool ClientFightEngine::dropKOOtherMonster()
{
    bool commonReturn=CommonFightEngine::dropKOOtherMonster();

    bool battleReturn=false;
    if(!battleCurrentMonster.empty())
    {
        if(!battleStat.empty() && battleMonsterPlace.front()<battleStat.size())
        {
            battleCurrentMonster.erase(battleCurrentMonster.cbegin());
            battleStat[battleMonsterPlace.front()]=0x02;//not able to battle
            battleMonsterPlace.erase(battleMonsterPlace.cbegin());
        }
        battleReturn=true;
    }
    bool haveValidMonster=false;
    unsigned int index=0;
    while(index<battleStat.size())
    {
        if(battleStat.at(index)==0x01)
        {
            haveValidMonster=true;
            break;
        }
        index++;
    }
    if(!haveValidMonster)
        battleStat.clear();

    return commonReturn || battleReturn;
}

void ClientFightEngine::fightFinished()
{
    battleCurrentMonster.clear();
    battleStat.clear();
    battleMonsterPlace.clear();
    doTurnIfChangeOfMonster=true;
    CommonFightEngine::fightFinished();
}

bool ClientFightEngine::isInFight() const
{
    if(CommonFightEngine::isInFight())
        return true;
    else if(!battleStat.empty())
        return true;
    else
        return false;
}

void ClientFightEngine::resetAll()
{
    mLastGivenXP=0;
    fightId=0;

    randomSeeds.clear();
    player_informations_local.playerMonster.clear();
    fightEffectList.clear();

    battleCurrentMonster.clear();
    battleStat.clear();
    public_and_private_informations.encyclopedia_monster=NULL;
    public_and_private_informations.encyclopedia_item=NULL;
    client=NULL;

    CommonFightEngine::resetAll();
}

std::vector<uint8_t> ClientFightEngine::addPlayerMonster(const PlayerMonster &playerMonster)
{
    std::vector<PlayerMonster> monsterList;
    monsterList.push_back(playerMonster);

    return addPlayerMonster(monsterList);
}

std::vector<uint8_t> ClientFightEngine::addPlayerMonster(const std::vector<PlayerMonster> &playerMonster)
{
    std::vector<uint8_t> positionList;
    const uint8_t basePosition=static_cast<uint8_t>(public_and_private_informations.playerMonster.size());
    Player_private_and_public_informations &informations=client->get_player_informations();
    unsigned int index=0;
    while(index<playerMonster.size())
    {
        const uint16_t &monsterId=playerMonster.at(index).monster;
        if(informations.encyclopedia_monster!=NULL)
            informations.encyclopedia_monster[monsterId/8]|=(1<<(7-monsterId%8));
        else
            std::cerr << "ClientFightEngine::addPlayerMonster(std::vector): encyclopedia_monster is null, unable to set" << std::endl;
        positionList.push_back(basePosition+static_cast<uint8_t>(index));
        index++;
    }
    CommonFightEngine::addPlayerMonster(playerMonster);
    return positionList;
}

bool ClientFightEngine::internalTryEscape()
{
    emit message("BaseWindow::on_toolButtonFightQuit_clicked(): emit tryEscape()");
    client->tryEscape();
    return CommonFightEngine::internalTryEscape();
}

bool ClientFightEngine::tryCatchClient(const uint16_t &item)
{
    if(!playerMonster_catchInProgress.empty())
    {
        error("Añready try catch in progress");
        return false;
    }
    if(wildMonsters.empty())
    {
        error("Try catch when not with wild");
        return false;
    }
    emit message("ClientFightEngine::tryCapture(): emit tryCapture()");
    client->useObject(item);
    PlayerMonster newMonster;
    newMonster.buffs=wildMonsters.front().buffs;
    newMonster.catched_with=item;
    newMonster.egg_step=0;
    newMonster.gender=wildMonsters.front().gender;
    newMonster.hp=wildMonsters.front().hp;
    #ifndef CATCHCHALLENGER_VERSION_SINGLESERVER
    newMonster.id=0;//unknown at this time
    #endif
    newMonster.level=wildMonsters.front().level;
    newMonster.monster=wildMonsters.front().monster;
    newMonster.remaining_xp=0;
    newMonster.skills=wildMonsters.front().skills;
    newMonster.sp=0;
    playerMonster_catchInProgress.push_back(newMonster);
    //need wait the server reply, monsterCatch(const bool &success)
    return true;
}

bool ClientFightEngine::catchInProgress() const
{
    return !playerMonster_catchInProgress.empty();
}

uint32_t ClientFightEngine::catchAWild(const bool &toStorage, const PlayerMonster &newMonster)
{
    Q_UNUSED(toStorage);
    Q_UNUSED(newMonster);
    return 0;
}

Skill::AttackReturn ClientFightEngine::doTheCurrentMonsterAttack(const uint16_t &skill,const uint8_t &skillLevel)
{
    fightEffectList.push_back(CommonFightEngine::doTheCurrentMonsterAttack(skill,skillLevel));
    return fightEffectList.back();
}

bool ClientFightEngine::applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn)
{
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
    {
        emit message("No current monster at apply current life effect");
        return false;
    }
    #ifdef DEBUG_CLIENT_BATTLE
    emit message("applyCurrentLifeEffectReturn on: "+QString::number(effectReturn.on));
    #endif
    int32_t quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
    switch(effectReturn.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        {

            PublicPlayerMonster *publicPlayerMonster;
            if(!wildMonsters.empty())
                publicPlayerMonster=&wildMonsters.front();
            else if(!botFightMonsters.empty())
                publicPlayerMonster=&botFightMonsters.front();
            else if(!battleCurrentMonster.empty())
                publicPlayerMonster=&battleCurrentMonster.front();
            else
            {
                emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"unknown other monster type");
                return false;
            }
            quantity=effectReturn.quantity;
            qDebug() << "applyCurrentLifeEffect() add hp on the ennemy " << quantity;
            if(quantity<0 && (-quantity)>(int32_t)publicPlayerMonster->hp)
            {
                qDebug() << "applyCurrentLifeEffect() ennemy is KO";
                publicPlayerMonster->hp=0;
                ableToFight=false;
            }
            else if(quantity>0 && quantity>(int32_t)(stat.hp-publicPlayerMonster->hp))
            {
                qDebug() << "applyCurrentLifeEffect() ennemy is fully healled";
                publicPlayerMonster->hp=stat.hp;
            }
            else
                publicPlayerMonster->hp+=quantity;
        }
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            quantity=effectReturn.quantity;
            qDebug() << "applyCurrentLifeEffect() add hp " << quantity;
            if(quantity<0 && (-quantity)>(int32_t)playerMonster->hp)
            {
                qDebug() << "applyCurrentLifeEffect() current monster is KO";
                playerMonster->hp=0;
            }
            else if(quantity>0 && quantity>(int32_t)(stat.hp-playerMonster->hp))
            {
                qDebug() << "applyCurrentLifeEffect() you are fully healled";
                playerMonster->hp=stat.hp;
            }
            else
                playerMonster->hp+=quantity;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
    return true;
}

void ClientFightEngine::addAndApplyAttackReturnList(const std::vector<Skill::AttackReturn> &attackReturnList)
{
    qDebug() << "addAndApplyAttackReturnList()";
    unsigned int index=0;
    unsigned int sub_index=0;
    while(index<attackReturnList.size())
    {
        const Skill::AttackReturn &attackReturn=attackReturnList.at(index);
        if(attackReturn.success)
        {
            sub_index=0;
            while(sub_index<attackReturn.addBuffEffectMonster.size())
            {
                Skill::BuffEffect buff=attackReturn.addBuffEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buff.on=invertApplyOn(buff.on);
                qDebug() << "addAndApplyAttackReturnList() buff on " << buff.on << ", buff:" << buff.buff << ", buff level:" << buff.level;
                addCurrentBuffEffect(buff);
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.removeBuffEffectMonster.size())
            {
                Skill::BuffEffect buff=attackReturn.removeBuffEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buff.on=invertApplyOn(buff.on);
                qDebug() << "addAndApplyAttackReturnList() buff on " << buff.on << ", buff:" << buff.buff << ", buff level:" << buff.level;
                removeBuffEffectFull(buff);
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.lifeEffectMonster.size())
            {
                Skill::LifeEffectReturn lifeEffect=attackReturn.lifeEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    lifeEffect.on=invertApplyOn(lifeEffect.on);
                qDebug() << "addAndApplyAttackReturnList() life effect on " << lifeEffect.on << ", quantity:" << lifeEffect.quantity;
                if(!applyCurrentLifeEffectReturn(lifeEffect))
                {
                    emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                                  "Error applying the life effect");
                    return;
                }
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.buffLifeEffectMonster.size())
            {
                Skill::LifeEffectReturn buffEffect=attackReturn.buffLifeEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buffEffect.on=invertApplyOn(buffEffect.on);
                qDebug() << "addAndApplyAttackReturnList() life effect on " << buffEffect.on << ", quantity:" << buffEffect.quantity;
                if(!applyCurrentLifeEffectReturn(buffEffect))
                {
                    emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                                  "Error applying the life effect");
                    return;
                }
                sub_index++;
            }
        }
        index++;
    }
    this->fightEffectList.insert(this->fightEffectList.cend(),attackReturnList.cbegin(),attackReturnList.cend());
}

PublicPlayerMonster * ClientFightEngine::getOtherMonster()
{
    if(!battleCurrentMonster.empty())
        return (PublicPlayerMonster *)&battleCurrentMonster.front();
    return CommonFightEngine::getOtherMonster();
}

uint8_t ClientFightEngine::getOtherSelectedMonsterNumber() const
{
    return 0;
}

std::vector<Skill::AttackReturn> ClientFightEngine::getAttackReturnList() const
{
    return fightEffectList;
}

Skill::AttackReturn ClientFightEngine::getFirstAttackReturn() const
{
    return fightEffectList.front();
}

Skill::AttackReturn ClientFightEngine::generateOtherAttack()
{
    fightEffectList.push_back(CommonFightEngine::generateOtherAttack());
    return fightEffectList.back();
}

void ClientFightEngine::removeTheFirstLifeEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().lifeEffectMonster.empty())
        fightEffectList.front().lifeEffectMonster.erase(fightEffectList.front().lifeEffectMonster.begin());
}

void ClientFightEngine::removeTheFirstBuffEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().buffLifeEffectMonster.empty())
        fightEffectList.front().buffLifeEffectMonster.erase(fightEffectList.front().buffLifeEffectMonster.begin());
}

void ClientFightEngine::removeTheFirstAddBuffEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().addBuffEffectMonster.empty())
        fightEffectList.front().addBuffEffectMonster.erase(fightEffectList.front().addBuffEffectMonster.begin());
}

void ClientFightEngine::removeTheFirstRemoveBuffEffectAttackReturn()
{
    if(fightEffectList.empty())
        return;
    if(!fightEffectList.front().removeBuffEffectMonster.empty())
        fightEffectList.front().removeBuffEffectMonster.erase(fightEffectList.front().removeBuffEffectMonster.begin());
}

void ClientFightEngine::removeTheFirstAttackReturn()
{
    if(!fightEffectList.empty())
        fightEffectList.erase(fightEffectList.cbegin());
}

bool ClientFightEngine::firstAttackReturnHaveMoreEffect()
{
    if(fightEffectList.empty())
        return false;
    if(!fightEffectList.front().lifeEffectMonster.empty())
        return true;
    if(!fightEffectList.front().addBuffEffectMonster.empty())
        return true;
    if(!fightEffectList.front().removeBuffEffectMonster.empty())
        return true;
    if(!fightEffectList.front().buffLifeEffectMonster.empty())
        return true;
    return false;
}

bool ClientFightEngine::firstLifeEffectQuantityChange(int32_t quantity)
{
    if(fightEffectList.empty())
    {
        emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"try add quantity to non existant life effect");
        return false;
    }
    if(fightEffectList.front().lifeEffectMonster.empty())
    {
        emit newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"try add quantity to life effect list empty");
        return false;
    }
    fightEffectList.front().lifeEffectMonster.front().quantity+=quantity;
    return true;
}

void ClientFightEngine::setVariableContent(Player_private_and_public_informations player_informations_local)
{
    this->player_informations_local=player_informations_local;
    updateCanDoFight();
}

bool ClientFightEngine::isInBattle() const
{
    return !battleStat.empty();
}

bool ClientFightEngine::haveBattleOtherMonster() const
{
    return !battleCurrentMonster.empty();
}

bool ClientFightEngine::useSkill(const uint16_t &skill)
{
    const bool wasInBotFight=!botFightMonsters.empty();
    mLastGivenXP=0;
    client->useSkill(skill);
    if(!isInBattle())
    {
        if(!CommonFightEngine::useSkill(skill))
            return false;
        return finishTheTurn(wasInBotFight);
    }
    else
        return true;
    ////> drop model server bool Client::useSkill(const uint16_t &skill)
}

bool ClientFightEngine::finishTheTurn(const bool &isBot)
{
    const bool &win=!currentMonsterIsKO() && otherMonsterIsKO();
    if(!isInFight())
    {
        if(win)
        {
            if(isBot)
            {
                if(public_and_private_informations.bot_already_beaten!=NULL)
                {
                    public_and_private_informations.bot_already_beaten[fightId/8]|=(1<<(7-fightId%8));
                    fightId=0;
                }
                else
                {
                    std::cerr << "ClientFightEngine::finishTheTurn() public_and_private_informations.bot_already_beaten==NULL: "+std::to_string(fightId) << std::endl;
                    abort();
                }
                std::cout << public_and_private_informations.public_informations.pseudo <<
                             ": Register the win against the bot fight: "+std::to_string(fightId) << std::endl;
            }
        }
    }
    return win;
}

void ClientFightEngine::catchIsDone()
{
    wildMonsters.erase(wildMonsters.begin());
}

bool ClientFightEngine::doTheOtherMonsterTurn()
{
    if(!isInBattle())
        return CommonFightEngine::doTheOtherMonsterTurn();
    return true;
}

void ClientFightEngine::levelUp(const uint8_t &level, const uint8_t &monsterIndex)//call after done the level
{
    CommonFightEngine::levelUp(level,monsterIndex);
    const PlayerMonster &monster=public_and_private_informations.playerMonster.at(monsterIndex);
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(monster.monster);
    unsigned int index=0;
    while(index<monsterInformations.evolutions.size())
    {
        const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
        if(evolution.type==Monster::EvolutionType_Level && evolution.data.level==level)//the current monster is not updated
        {
            mEvolutionByLevelUp.push_back(monsterIndex);
            return;
        }
        index++;
    }
}

PlayerMonster * ClientFightEngine::evolutionByLevelUp()
{
    if(mEvolutionByLevelUp.empty())
        return NULL;
    uint8_t monsterIndex=mEvolutionByLevelUp.front();
    mEvolutionByLevelUp.erase(mEvolutionByLevelUp.cbegin());
    return &public_and_private_informations.playerMonster[monsterIndex];
}

uint8_t ClientFightEngine::getPlayerMonsterPosition(const PlayerMonster * const playerMonster)
{
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(&public_and_private_informations.playerMonster.at(index)==playerMonster)
            return static_cast<uint8_t>(index);
        index++;
    }
    std::cerr << "getPlayerMonsterPosition() with not existing monster" << std::endl;
    return 0;
}

void ClientFightEngine::addToEncyclopedia(const uint16_t &monster)
{
    Player_private_and_public_informations &informations=client->get_player_informations();
    if(informations.encyclopedia_monster!=NULL)
        informations.encyclopedia_monster[monster/8]|=(1<<(7-monster%8));
    else
        std::cerr << "ClientFightEngine::addPlayerMonster(PlayerMonster): encyclopedia_monster is null, unable to set" << std::endl;
}

void ClientFightEngine::confirmEvolutionByPosition(const uint8_t &monterPosition)
{
    client->confirmEvolutionByPosition(monterPosition);
    CatchChallenger::PlayerMonster &playerMonster=public_and_private_informations.playerMonster[monterPosition];
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters[playerMonster.monster];
    unsigned int sub_index=0;
    while(sub_index<monsterInformations.evolutions.size())
    {
        if(monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Level)
        {
            playerMonster.monster=monsterInformations.evolutions.at(sub_index).evolveTo;
            addToEncyclopedia(playerMonster.monster);
            Monster::Stat stat=getStat(monsterInformations,playerMonster.level);
            if(playerMonster.hp>stat.hp)
                playerMonster.hp=stat.hp;
            return;
        }
        sub_index++;
    }
    qDebug() << "Evolution not found";
    return;
}

//return true if change level, multiplicator do at datapack loading
bool ClientFightEngine::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=CommonFightEngine::giveXPSP(xp,sp);
    mLastGivenXP=xp;
    return haveChangeOfLevel;
}

uint32_t ClientFightEngine::lastGivenXP()
{
    uint32_t tempLastGivenXP=mLastGivenXP;
    mLastGivenXP=0;
    return tempLastGivenXP;
}

void ClientFightEngine::errorFightEngine(const std::string &errorMessage)
{
    std::cerr << errorMessage << std::endl;
    error(errorMessage);
}

void ClientFightEngine::messageFightEngine(const std::string &message) const
{
    std::cout << message << std::endl;
}

uint32_t ClientFightEngine::randomSeedsSize() const
{
    return randomSeeds.size();
}

uint8_t ClientFightEngine::getOneSeed(const uint8_t &max)
{
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    std::cout << "Random seed remaining: " << randomSeeds.size() << std::endl;
    #endif
    const uint8_t &number=randomSeeds.at(0);
    randomSeeds.erase(randomSeeds.cbegin());
    return number%(max+1);
}

void ClientFightEngine::newRandomNumber(const std::string &data)
{
    randomSeeds.append(data);
}

void ClientFightEngine::setClient(Api_protocol * client)
{
    this->client=client;
}

//duplicate to have a return
bool ClientFightEngine::useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition)
{
    PlayerMonster * playerMonster=monsterByPosition(monsterPosition);
    if(playerMonster==NULL)
    {
        std::cerr << "Unable to locate the monster to use the item: " << std::to_string(monsterPosition) << std::endl;
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(object)!=CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend())
    {
    }
    //duplicate to have a return
    else if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend()
            ||
            CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend())
    {
        if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
        {
            //duplicate to have a return
            Skill::AttackReturn attackReturn;
            attackReturn.doByTheCurrentMonster=true;
            attackReturn.attackReturnCase=Skill::AttackReturnCase::AttackReturnCase_ItemUsage;
            //normal attack
            attackReturn.success=true;
            attackReturn.attack=0;
            //change monster if monsterPlace !=0
            attackReturn.monsterPlace=0;
            //use objet on monster if item!=0
            attackReturn.on_current_monster=true;
            attackReturn.item=object;

            const Monster::Stat &playerMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
            const std::vector<MonsterItemEffect> monsterItemEffect = CommonDatapack::commonDatapack.items.monsterItemEffect.at(object);
            unsigned int index=0;
            //duplicate to have a return
            while(index<monsterItemEffect.size())
            {
                const MonsterItemEffect &effect=monsterItemEffect.at(index);
                switch(effect.type)
                {
                    //duplicate to have a return
                    case MonsterItemEffectType_AddHp:
                        if(effect.data.hp>0 && (playerMonsterStat.hp-playerMonster->hp)>(uint32_t)effect.data.hp)
                        {
                            hpChange(playerMonster,playerMonster->hp+effect.data.hp);
                            Skill::LifeEffectReturn lifeEffectReturn;
                            lifeEffectReturn.quantity=effect.data.hp;
                            lifeEffectReturn.on=ApplyOn_Themself;
                            lifeEffectReturn.critical=false;
                            lifeEffectReturn.effective=1;
                            attackReturn.lifeEffectMonster.push_back(lifeEffectReturn);
                        }
                        //duplicate to have a return
                        else if(playerMonsterStat.hp!=playerMonster->hp)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            lifeEffectReturn.quantity=playerMonsterStat.hp-playerMonster->hp;
                            hpChange(playerMonster,playerMonsterStat.hp);
                            lifeEffectReturn.on=ApplyOn_Themself;
                            lifeEffectReturn.critical=false;
                            lifeEffectReturn.effective=1;
                            attackReturn.lifeEffectMonster.push_back(lifeEffectReturn);
                        }
                        else if(monsterItemEffect.size()==1)
                            return false;
                    break;
                    //duplicate to have a return
                    case MonsterItemEffectType_RemoveBuff:
                        if(effect.data.buff>0)
                        {
                            if(removeBuffOnMonster(playerMonster,effect.data.buff))
                            {
                                Skill::BuffEffect buffEffect;
                                buffEffect.buff=effect.data.buff;
                                buffEffect.on=ApplyOn_Themself;
                                buffEffect.level=1;
                                attackReturn.removeBuffEffectMonster.push_back(buffEffect);
                            }
                        }
                        else
                        {
                            if(!playerMonster->buffs.empty())
                            {
                                unsigned int index=0;
                                while(index<playerMonster->buffs.size())
                                {
                                    const PlayerBuff &playerBuff=playerMonster->buffs.at(index);
                                    Skill::BuffEffect buffEffect;
                                    buffEffect.buff=playerBuff.buff;
                                    buffEffect.on=ApplyOn_Themself;
                                    buffEffect.level=playerBuff.level;
                                    attackReturn.removeBuffEffectMonster.push_back(buffEffect);
                                    index++;
                                }
                            }
                            removeAllBuffOnMonster(playerMonster);
                        }
                    break;
                    default:
                        messageFightEngine(QStringLiteral("Item %1 have unknown effect").arg(object).toStdString());
                    break;
                }
                index++;
            }
            //duplicate to have a return
            if(isInFight())
            {
                doTheOtherMonsterTurn();
                this->fightEffectList.push_back(attackReturn);
            }
            return true;
        }
        else if(CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(object)!=CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend())
        {
        }
    }
    else if(CommonDatapack::commonDatapack.items.itemToLearn.find(object)!=CommonDatapack::commonDatapack.items.itemToLearn.cend())
    {
    }
    else
        return false;

    return CommonFightEngine::useObjectOnMonsterByPosition(object,monsterPosition);
}
