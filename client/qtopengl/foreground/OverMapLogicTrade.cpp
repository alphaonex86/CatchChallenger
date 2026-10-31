#include "OverMapLogic.hpp"
#include "../above/Trade.hpp"
#include "../ConnexionManager.hpp"

void OverMapLogic::ensureTrade()
{
    if(trade==nullptr)
    {
        trade=new Trade();
        if(!connect(trade,&Trade::setAbove,this,&OverMapLogic::setAbove))
            abort();
        if(!connect(trade,&Trade::add_to_inventory,this,&OverMapLogic::add_to_inventory_slotpair))
            abort();
        if(!connect(trade,&Trade::addCash,this,&OverMapLogic::addCash))
            abort();
        if(!connect(trade,&Trade::removeCash,this,&OverMapLogic::removeCash))
            abort();
        if(!connect(trade,&Trade::showTip,this,&OverMapLogic::showTip))
            abort();
        if(!connect(trade,&Trade::selectObjectForTrade,this,&OverMapLogic::tradeSelectObject))
            abort();
        if(!connect(trade,&Trade::selectMonsterForTrade,this,&OverMapLogic::tradeSelectMonster))
            abort();
    }
}

void OverMapLogic::tradeRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    tradeRequests.push_back(std::pair<std::string,uint8_t>(pseudo,skinInt));
    if(!tradePromptShown)
        tradePromptShow();
}

//no modal dialog here: the in-game dialog asks, closing or replacing it refuses
void OverMapLogic::tradePromptShow()
{
    if(tradeRequests.empty())
        return;
    setIG_dialog(tr("%1 wants to trade with you").arg(QString::fromStdString(tradeRequests.front().first))+
                 QStringLiteral("<br/><a href=\"tradeaccept\">%1</a><br/><a href=\"traderefuse\">%2</a>").arg(tr("Accept")).arg(tr("Refuse")),
                 QString::fromStdString(tradeRequests.front().first));
    tradePromptShown=true;
}

//accept: the trade opens, the other requests are refused; refuse: ask the next one
void OverMapLogic::tradePromptAnswer(const bool &accept)
{
    tradePromptShown=false;
    if(tradeRequests.empty())
        return;
    const std::pair<std::string,uint8_t> request=tradeRequests.front();
    tradeRequests.erase(tradeRequests.cbegin());
    if(accept)
    {
        connexionManager->client->tradeAccepted();
        while(!tradeRequests.empty())
        {
            connexionManager->client->tradeRefused();
            tradeRequests.erase(tradeRequests.cbegin());
        }
        tradeAcceptedByOther(request.first,request.second);
    }
    else
    {
        connexionManager->client->tradeRefused();
        tradePromptShow();
    }
}

void OverMapLogic::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
{
    ensureTrade();
    trade->setVar(connexionManager,pseudo,skinInt);
    setAbove(trade);
}

void OverMapLogic::tradeCanceledByOther()
{
    if(trade!=nullptr)
        trade->tradeCanceledByOther();
}

void OverMapLogic::tradeFinishedByOther()
{
    if(trade!=nullptr)
        trade->tradeFinishedByOther();
}

void OverMapLogic::tradeValidatedByTheServer()
{
    if(trade==nullptr)
        return;
    trade->tradeValidatedByTheServer();
    //a monster received with a trade evolution
    checkEvolution();
}

void OverMapLogic::tradeAddTradeCash(const uint64_t &cash)
{
    if(trade!=nullptr)
        trade->tradeAddTradeCash(cash);
}

void OverMapLogic::tradeAddTradeObject(const uint16_t &item,const uint32_t &quantity)
{
    if(trade!=nullptr)
        trade->tradeAddTradeObject(item,quantity);
}

void OverMapLogic::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    if(trade!=nullptr)
        trade->tradeAddTradeMonster(monster);
}

void OverMapLogic::tradeSelectObject()
{
    selectObject(ObjectType_Trade);
}

void OverMapLogic::tradeSelectMonster()
{
    selectObject(ObjectType_MonsterToTrade);
}
