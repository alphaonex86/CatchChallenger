#include "Quests.hpp"
#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../ConnexionManager.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../libqtcatchchallenger/Ultimate.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonDatapackServerSpec.hpp"
#include <QPainter>
#include <QBuffer>
#include <iostream>

Quests::Quests() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    label.setPixmap(QPixmap(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);
    connect(quit,&CustomButton::clicked,this,&Quests::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    next=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    proxy=new QGraphicsProxyWidget(wdialog);
    questsList=new QListWidget();
    proxy->setWidget(questsList);

    questDetails=new QGraphicsTextItem(wdialog);
    cancelButton=new CustomButton(":/CC/images/interface/redbutton.png",wdialog);

    if(!connect(&Language::language,&Language::newLanguage,this,&Quests::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&Quests::removeAbove,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&Quests::sendNext,Qt::QueuedConnection))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Quests::sendBack,Qt::QueuedConnection))
        abort();
    if(!connect(cancelButton,&CustomButton::clicked,this,&Quests::cancel_slot,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Quests::~Quests()
{
    delete wdialog;
    delete quit;
    delete title;
}

void Quests::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Quests::boundingRect() const
{
    return QRectF();
}

void Quests::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    int idealW=900;
    int idealH=400;
    if(idealH<widget->height()-200)
        idealH=widget->height()-200;
    unsigned int space=30;
    if(widget->width()<800 || widget->height()<600)
    {
        idealW/=2;
        idealH/=2;
        space/=2;
    }
    if(idealW>widget->width())
        idealW=widget->width();
    int top=36;
    int bottom=94/2;
    if(widget->width()<800 || widget->height()<600)
    {
        top/=2;
        bottom/=2;
    }
    if((idealH+top+bottom)>widget->height())
        idealH=widget->height()-(top+bottom);

    int x=widget->width()/2-idealW/2;
    int y=widget->height()/2-idealH/2;

    wdialog->setPos(x,y);
    wdialog->setSize(idealW,idealH);

    auto font=questsList->font();
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        back->setSize(83/2,94/2);
        next->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        back->setSize(83,94);
        next->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);
    proxy->setPos(x+wdialog->currentBorderSize()+space,y+label.pixmap().height()/2+space);
    back->setPos(widget->width()/2-label.pixmap().width()/2*label.scale()-back->width(),label.y());
    next->setPos(widget->width()/2+label.pixmap().width()/2*label.scale(),label.y());

    questsList->setFixedSize(idealW/2-space-space/2,idealH-space*2-label.pixmap().height()/2*label.scale());
    proxy->setPos(space,space+label.pixmap().height()/2*label.scale());
    questDetails->setPos(space+idealW/2+space,space+label.pixmap().height()/2*label.scale());
    questDetails->setTextWidth(idealW/2-space*2-label.pixmap().height()/2*label.scale());
    cancelButton->setPos(idealW*3/4-cancelButton->width()/2,idealH-space-cancelButton->height());

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Quests::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Quests::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Quests::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Quests::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Quests::newLanguage()
{
    title->setText(tr("Quests"));
    cancelButton->setText(tr("Cancel"));
}

void Quests::setVar(ConnexionManager *connexionManager)
{
    this->connexionManager=connexionManager;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    questsList->clear();
    auto i=playerInformations.quests.begin();
    while(i!=playerInformations.quests.cend())
    {
        const uint16_t questId=i->first;
        if(QtDatapackClientLoader::datapackLoader->get_questsExtra().find(questId)!=QtDatapackClientLoader::datapackLoader->get_questsExtra().cend())
        {
            if(i->second.step>0)
            {
                QListWidgetItem * item=new QListWidgetItem(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId).name));
                questsList->addItem(item);
                item->setData(99,questId);
            }
        }
        ++i;
    }
    on_questsList_itemSelectionChanged();
}

void Quests::cancel_slot()
{
    QList<QListWidgetItem *> items=questsList->selectedItems();
    if(items.size()!=1)
        return;
    connexionManager->client->cancelQuest(items.first()->data(99).toInt());
    on_questsList_itemSelectionChanged();
}

void Quests::on_questsList_itemSelectionChanged()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    QList<QListWidgetItem *> items=questsList->selectedItems();
    if(items.size()!=1)
    {
        questDetails->setHtml(tr("Select a quest"));
        return;
    }
    const uint16_t &questId=items.first()->data(99).toUInt();
    if(playerInformations.quests.find(questId)==playerInformations.quests.cend())
    {
        qDebug() << "Selected quest is not into the player list";
        questDetails->setHtml(tr("Select a quest"));
        return;
    }
    const QtDatapackClientLoader::QuestExtra &questExtra=QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId);
    if(playerInformations.quests.at(questId).step==0 ||
            playerInformations.quests.at(questId).step>questExtra.steps.size())
    {
        qDebug() << "Selected quest step is out of range";
        questDetails->setHtml(tr("Select a quest"));
        return;
    }
    const uint8_t stepQuest=playerInformations.quests.at(questId).step-1;
    std::string stepDescription;
    {
        if(stepQuest>=questExtra.steps.size())
        {
            qDebug() << "no condition match into stepDescriptionList";
            questDetails->setHtml(tr("Select a quest"));
            return;
        }
        else
            stepDescription=questExtra.steps.at(stepQuest);
    }
    stepDescription+="<br />";
    std::string stepRequirements;
    {
        std::vector<CatchChallenger::Quest::Item> items=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId)
                .steps.at(playerInformations.quests.at(questId).step-1).requirements.items;
        std::vector<std::string> objects;
        unsigned int index=0;
        while(index<items.size())
        {
            QPixmap image;
            std::string name;
            if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(items.at(index).item)!=QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
            {
                image=QPixmap(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(items.at(index).item).imagePath));
                name=QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(items.at(index).item).name;
            }
            else
            {
                image=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
                name="id: "+std::to_string(items.at(index).item);
            }

            image=image.scaled(24,24);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG");
            if(objects.size()<16)
            {
                if(items.at(index).quantity>1)
                    objects.push_back(QStringLiteral("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />")
                                      .arg(QString(byteArray.toBase64()))
                                      .arg(items.at(index).quantity).arg(QString::fromStdString(name))
                                      .toStdString());
                else
                    objects.push_back(QStringLiteral("%2 <img src=\"data:image/png;base64,%1\" />")
                                      .arg(QString(byteArray.toBase64()))
                                      .arg(QString::fromStdString(name)).toStdString());
            }
            index++;
        }
        if(objects.size()==16)
            objects.push_back("...");
        stepRequirements+=tr("Step requirements: ").toStdString()+"<br />"+stringimplode(objects,", ");
    }
    std::string finalRewards;
    if(questExtra.showRewards || Ultimate::ultimate.isUltimate())
    {
        finalRewards+=tr("Final rewards: ").toStdString();
        {
            std::vector<CatchChallenger::Quest::Item> items=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId).rewards.items;
            std::vector<std::string> objects;
            unsigned int index=0;
            while(index<items.size())
            {
                QPixmap image;
                std::string name;
                if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(items.at(index).item)!=
                        QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
                {
                    image=QPixmap(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(items.at(index).item).imagePath));
                    name=QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(items.at(index).item).name;
                }
                else
                {
                    image=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
                    name=QStringLiteral("id: %1").arg(items.at(index).item).toStdString();
                }
                image=image.scaled(24,24);
                QByteArray byteArray;
                QBuffer buffer(&byteArray);
                image.save(&buffer, "PNG");
                if(objects.size()<16)
                {
                    if(items.at(index).quantity>1)
                        objects.push_back(QStringLiteral("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />")
                                          .arg(QString(byteArray.toBase64()))
                                          .arg(items.at(index).quantity)
                                          .arg(QString::fromStdString(name))
                                          .toStdString());
                    else
                        objects.push_back(QStringLiteral("%2 <img src=\"data:image/png;base64,%1\" />")
                                          .arg(QString(byteArray.toBase64()))
                                          .arg(QString::fromStdString(name))
                                          .toStdString());
                }
                index++;
            }
            if(objects.size()==16)
                objects.push_back("...");
            finalRewards+=stringimplode(objects,", ")+"<br />";
        }
        {
            std::vector<CatchChallenger::ReputationRewards> reputationRewards=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId).rewards.reputation;
            std::vector<std::string> reputations;
            unsigned int index=0;
            while(index<reputationRewards.size())
            {
                if(QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(
                                                       reputationRewards.at(index).reputationId).name
                            )!=
                        QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend())
                {
                    const std::string &reputationName=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(
                                reputationRewards.at(index).reputationId).name;
                    if(reputationRewards.at(index).point<0)
                        reputations.push_back(tr("Less reputation for %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(reputationName).name)).toStdString());
                    if(reputationRewards.at(index).point>0)
                        reputations.push_back(tr("More reputation for %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(reputationName).name)).toStdString());
                }
                index++;
            }
            if(reputations.size()>16)
            {
                while(reputations.size()>15)
                    reputations.pop_back();
                reputations.push_back("...");
            }
            finalRewards+=stringimplode(reputations,", ")+"<br />";
        }
        {
            std::vector<CatchChallenger::ActionAllow> allowRewards=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().at(questId).rewards.allow;
            std::vector<std::string> allows;
            unsigned int index=0;
            while(index<allowRewards.size())
            {
                if(vectorcontainsAtLeastOne(allowRewards,CatchChallenger::ActionAllow_Clan))
                    allows.push_back(tr("Add permission to create clan").toStdString());
                index++;
            }
            if(allows.size()>16)
            {
                while(allows.size()>15)
                    allows.pop_back();
                allows.push_back("...");
            }
            finalRewards+=stringimplode(allows,", ")+"<br />";
        }
    }

    questDetails->setHtml(QString::fromStdString(stepDescription+stepRequirements+finalRewards));
}
