#include "SocialChat.h"
#include "ui_SocialChat.h"

#include <QListWidget>

SocialChat * SocialChat::socialChat=NULL;

SocialChat::SocialChat() :
    ui(new Ui::SocialChat)
{
    ui->setupUi(this);
}

void SocialChat::showEvent(QShowEvent * event)
{
    QHash<CatchChallenger::Api_protocol *,ActionsBotInterface::Player>::const_iterator i = ActionsBotInterface::clientList.constBegin();
    while (i != ActionsBotInterface::clientList.constEnd()) {
        ActionsBotInterface::Player &client=const_cast<ActionsBotInterface::Player &>(i.value());
        if(client.api->getCaracterSelected())
        {
            const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client.api->get_player_informations();
            const QString &qtpseudo=QString::fromStdString(player_private_and_public_informations.public_informations.pseudo);
            ui->bots->addItem(qtpseudo);
            pseudoToBot[qtpseudo]=&client;
        }
        ++i;
    }

    if(!event->spontaneous())
    {
        if(ui->bots->count()==1)
        {
            QListWidgetItem * item=ui->bots->itemAt(0,0);
            if(!item->isSelected())
            {
                item->setSelected(true);
                on_bots_itemSelectionChanged();
            }
            ui->groupBoxBot->setVisible(false);
        }
        std::cout << "QShowEvent: QEvent::Type: " << (int)event->type() << std::endl;
    }
    QWidget::showEvent(event);
}

SocialChat::~SocialChat()
{
    delete ui;
}

void SocialChat::on_bots_itemSelectionChanged()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    ActionsBotInterface::Player * client=pseudoToBot.value(pseudo);

    ui->groupBoxVisiblePlayers->setEnabled(true);
    ui->groupBoxChat->setEnabled(true);
    ui->groupBoxInformations->setEnabled(true);

    //loadPlayerInformation();
}
