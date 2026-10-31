#include "EvolutionScreen.h"
#include "../../../libcatchchallenger/MonsterEvolution.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

#include <QVBoxLayout>

EvolutionScreen::EvolutionScreen(QWidget *parent) :
    QWidget(parent),
    image_(new QLabel(this)),
    text_(new QLabel(this)),
    button_(new QPushButton(this)),
    state_(State_Evolved)
{
    QVBoxLayout *layout=new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(image_,0,Qt::AlignHCenter);
    layout->addWidget(text_,0,Qt::AlignHCenter);
    layout->addWidget(button_,0,Qt::AlignHCenter);
    layout->addStretch();
    image_->setMinimumSize(160,160);
    image_->setAlignment(Qt::AlignCenter);
    timer_.setInterval(50);
    if(!connect(&timer_,&QTimer::timeout,this,&EvolutionScreen::tick))
        abort();
    if(!connect(button_,&QPushButton::clicked,this,&EvolutionScreen::buttonClicked))
        abort();
}

QString EvolutionScreen::monsterName(const CATCHCHALLENGER_TYPE_MONSTER &monster)
{
    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(monster))
        return QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster).name);
    return QString::number(monster);
}

void EvolutionScreen::start(const CATCHCHALLENGER_TYPE_MONSTER &from,const CATCHCHALLENGER_TYPE_MONSTER &to,const bool &canCancel)
{
    fromImage_=QtDatapackClientLoader::datapackLoader->getMonsterExtra(from).front.scaled(160,160,Qt::KeepAspectRatio);
    toImage_=QtDatapackClientLoader::datapackLoader->getMonsterExtra(to).front.scaled(160,160,Qt::KeepAspectRatio);
    fromName_=monsterName(from);
    toName_=monsterName(to);
    state_=State_Evolving;
    image_->setPixmap(fromImage_);
    text_->setText(tr("%1 is evolving!").arg(fromName_));
    button_->setText(tr("Cancel"));
    button_->setVisible(canCancel);
    elapsed_.start();
    timer_.start();
}

void EvolutionScreen::tick()
{
    const uint32_t elapsedMs=static_cast<uint32_t>(elapsed_.elapsed());
    if(elapsedMs>=CatchChallenger::MonsterEvolution::animationMs)
        showEnd(State_Evolved);
    else if(CatchChallenger::MonsterEvolution::showEvolved(elapsedMs))
        image_->setPixmap(toImage_);
    else
        image_->setPixmap(fromImage_);
}

void EvolutionScreen::showEnd(const State &state)
{
    timer_.stop();
    state_=state;
    if(state==State_Evolved)
    {
        image_->setPixmap(toImage_);
        text_->setText(tr("%1 evolved into %2!").arg(fromName_).arg(toName_));
    }
    else
    {
        image_->setPixmap(fromImage_);
        text_->setText(tr("%1 stopped evolving.").arg(fromName_));
    }
    button_->setText(tr("OK"));
    button_->setVisible(true);
    if(state==State_Evolved)
        emit evolved();
}

void EvolutionScreen::buttonClicked()
{
    if(state_==State_Evolving)
        showEnd(State_Canceled);
    else
        emit closed();
}
