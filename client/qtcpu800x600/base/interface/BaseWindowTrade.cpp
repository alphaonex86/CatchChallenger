#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
{
    ui->stackedWidget->setCurrentWidget(ui->page_trade);
    tradeOtherStat=TradeOtherStat_InWait;
    //reset the current player info
    ui->tradePlayerCash->setMinimum(0);
    ui->tradePlayerCash->setValue(0);
    ui->tradePlayerItems->clear();
    ui->tradePlayerMonsters->clear();
    ui->tradePlayerCash->setEnabled(true);
    ui->tradePlayerItems->setEnabled(true);
    ui->tradePlayerMonsters->setEnabled(true);
    ui->tradeAddItem->setEnabled(true);
    ui->tradeAddMonster->setEnabled(true);
    ui->tradeValidate->setEnabled(true);

    const QPixmap skin(getFrontSkin(skinInt));
    if(!skin.isNull())
    {
        //reset the other player info
        ui->tradePlayerImage->setVisible(true);
        ui->tradePlayerPseudo->setVisible(true);
        ui->tradeOtherImage->setVisible(true);
        ui->tradeOtherPseudo->setVisible(true);
        ui->tradeOtherImage->setPixmap(skin);
        ui->tradeOtherPseudo->setText(QString::fromStdString(pseudo));
    }
    else
    {
        ui->tradePlayerImage->setVisible(false);
        ui->tradePlayerPseudo->setVisible(false);
        ui->tradeOtherImage->setVisible(false);
        ui->tradeOtherPseudo->setVisible(false);
    }
    ui->tradeOtherCash->setValue(0);
    ui->tradeOtherItems->clear();
    ui->tradeOtherMonsters->clear();
    ui->tradeOtherStat->setText(tr("The other player don't have validate their selection"));
}

void BaseWindow::tradeCanceledByOther()
{
    if(ui->stackedWidget->currentWidget()!=ui->page_trade)
        return;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled your trade request").toStdString());
    addCash(ui->tradePlayerCash->value());
    add_to_inventory(tradeCurrentObjects,false);
    client->addPlayerMonster(tradeCurrentMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
}

void BaseWindow::tradeFinishedByOther()
{
    tradeOtherStat=TradeOtherStat_Accepted;
    if(!ui->tradeValidate->isEnabled())
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    else
        ui->tradeOtherStat->setText(tr("The other player have validated the selection"));
}

void BaseWindow::tradeValidatedByTheServer()
{
    if(ui->stackedWidget->currentWidget()==ui->page_trade)
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Your trade is successfull").toStdString());
    add_to_inventory(tradeOtherObjects);
    addCash(ui->tradeOtherCash->value());
    removeCash(ui->tradePlayerCash->value());
    tradeEvolutionMonsters=client->addPlayerMonster(tradeOtherMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
    checkEvolution();
}

void BaseWindow::tradeAddTradeCash(const uint64_t &cash)
{
    ui->tradeOtherCash->setValue(ui->tradeOtherCash->value()+static_cast<uint32_t>(cash));
}

void BaseWindow::tradeAddTradeObject(const uint16_t &item,const uint32_t &quantity)
{
    if(tradeOtherObjects.find(item)!=tradeOtherObjects.cend())
        tradeOtherObjects[item]+=quantity;
    else
        tradeOtherObjects[item]=quantity;
    ui->tradeOtherItems->clear();
    for(const auto &n : tradeOtherObjects) {
        ui->tradeOtherItems->addItem(itemToGraphic(n.first,n.second));
    }
}

void BaseWindow::tradeUpdateCurrentObject()
{
    ui->tradePlayerItems->clear();
    for(const auto &n : tradeCurrentObjects) {
        ui->tradePlayerItems->addItem(itemToGraphic(n.first,n.second));
    }
}
