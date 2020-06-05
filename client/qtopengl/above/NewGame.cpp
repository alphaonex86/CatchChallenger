#include "NewGame.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Settings.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../Language.hpp"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTextDocument>
#include <QDesktopServices>

NewGame::NewGame() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    ok=false;

    x=-1;
    y=-1;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",this);
    validate=new CustomButton(":/CC/images/interface/validate.png",this);
    connect(quit,&CustomButton::clicked,this,&NewGame::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    previous=new CustomButton(":/CC/images/interface/back.png",this);
    next=new CustomButton(":/CC/images/interface/next.png",this);
    uipseudo=new LineEdit(this);

    warning=new QGraphicsTextItem(this);
    warning->setVisible(false);

    if(!connect(&Language::language,&Language::newLanguage,this,&NewGame::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&NewGame::on_cancel_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(validate,&CustomButton::clicked,this,&NewGame::on_ok_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(previous,&CustomButton::clicked,this,&NewGame::on_previous_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&NewGame::on_next_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(uipseudo,&LineEdit::returnPressed,this,&NewGame::on_pseudo_returnPressed,Qt::QueuedConnection))
        abort();

    newLanguage();
}

NewGame::~NewGame()
{
    delete wdialog;
    delete quit;
    delete validate;
    delete title;
}

QRectF NewGame::boundingRect() const
{
    return QRectF();
}

void NewGame::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    bool forcedCenter=true;
    int idealW=900;
    int idealH=400;
    if(widget->width()<600 || widget->height()<480)
    {
        idealW/=2;
        idealH/=2;
    }
    if(idealW>widget->width())
        idealW=widget->width();
    int top=36;
    int bottom=94/2;
    if(widget->width()<600 || widget->height()<480)
    {
        top/=2;
        bottom/=2;
    }
    if((idealH+top+bottom)>widget->height())
        idealH=widget->height()-(top+bottom);

    if(x<0 || y<0 || forcedCenter)
    {
        x=widget->width()/2-idealW/2;
        y=widget->height()/2-idealH/2;
    }
    else
    {
        if((x+idealW)>widget->width())
            x=widget->width()-idealW;
        if((y+idealH+bottom)>widget->height())
            y=widget->height()-(idealH+bottom);
        if(y<top)
            y=top;
    }

    wdialog->setPos(x,y);
    wdialog->setSize(idealW,idealH);

    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        validate->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        previous->setSize(83,94);
        next->setSize(83,94);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        validate->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        previous->setSize(83,94);
        next->setSize(83,94);
    }

    //unsigned int nameBackgroundNewHeight=50;
    unsigned int space=30;
    if(widget->width()<600 || widget->height()<480)
    {
        space=10;
//        nameBackgroundNewHeight=50/2;
    }

    quit->setPos(x+(idealW/2-quit->width()-space/2),y+idealH-quit->height()/2);
    validate->setPos(x+(idealW/2+space/2),y+idealH-quit->height()/2);
    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    previous->setPos(x-previous->width()/2,widget->height()/2-previous->height()/2);
    int wTot=0;
    int index=0;
    while(index<centerImages.size())
    {
        if(index>0)
            wTot+=space;
        QGraphicsPixmapItem * p=centerImages.at(index);
        QPixmap pix=p->pixmap();
        if(p!=nullptr && !pix.isNull())
            wTot+=pix.width();
        index++;
    }
    int xTot=widget->width()/2-wTot/2;
    index=0;
    while(index<centerImages.size())
    {
        QGraphicsPixmapItem * p=centerImages.at(index);
        QPixmap pix=p->pixmap();
        if(p!=nullptr && !pix.isNull())
        {
            p->setPos(xTot,widget->height()/2-pix.height()/2);
            xTot+=pix.width();
        }
        if(index>0)
            xTot+=space;
        index++;
    }
    next->setPos(x-next->width()/2+idealW,widget->height()/2-previous->height()/2);
    uipseudo->setPos(widget->width()/2-uipseudo->width()/2,y+idealH-uipseudo->height()-space-validate->height()/2);
}

void NewGame::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    validate->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    previous->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void NewGame::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    validate->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    previous->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

/*void NewGame::setDatapack(std::string path)
{
    if(newProfile->getProfileCount()==1)
    {
        newProfile->on_ok_clicked();
        CharacterList::newProfileFinished();
        emit quitAbove();
    }
    else
        newProfile->show();
}*/

void NewGame::newLanguage()
{
     title->setText(tr("Select"));
}

bool NewGame::isOk() const
{
    return ok;
}

bool NewGame::haveSkin() const
{
    return skinList.size()>0;
}

void NewGame::on_cancel_clicked()
{
    ok=false;
    emit removeAbove();
}

void NewGame::setDatapack(const std::string &skinPath, const std::string &monsterPath, std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup, const std::vector<uint8_t> &forcedSkin)
{
    this->forcedSkin.clear();
    this->monstergroup.clear();
    this->step=Step1;
    this->skinLoaded=false;
    this->skinList.clear();
    this->skinListId.clear();

    this->forcedSkin=forcedSkin;
    this->monsterPath=monsterPath;
    this->monstergroup=monstergroup;
    ok=true;
    step=Step1;
    currentMonsterGroup=0;
    if(!monstergroup.empty())
        currentMonsterGroup=static_cast<uint8_t>(rand()%monstergroup.size());
    this->skinPath=skinPath;
    uint8_t index=0;
    while(index<CatchChallenger::CommonDatapack::commonDatapack.skins.size())
    {
        if(forcedSkin.empty() || vectorcontainsAtLeastOne(forcedSkin,(uint8_t)index))
        {
            const std::string &currentPath=skinPath+CatchChallenger::CommonDatapack::commonDatapack.skins.at(index);
            if(QFile::exists(QString::fromStdString(currentPath+"/back.png")) && QFile::exists(QString::fromStdString(currentPath+"/front.png")) && QFile::exists(QString::fromStdString(currentPath+"/trainer.png")))
            {
                skinList.push_back(CatchChallenger::CommonDatapack::commonDatapack.skins.at(index));
                skinListId.push_back(index);
            }
        }
        index++;
    }

    uipseudo->setMaxLength(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size);
    previous->setVisible(skinList.size()>=2);
    next->setVisible(skinList.size()>=2);

    currentSkin=0;
    if(!skinList.empty())
        currentSkin=static_cast<uint8_t>(rand()%skinList.size());
    updateSkin();
    uipseudo->setFocus();
    if(skinList.empty())
    {
        warning->setHtml(tr("No skin to select!"));
        warning->setVisible(true);
        return;
    }
}

void NewGame::updateSkin()
{
    skinLoaded=false;

    std::vector<std::string> paths;
    if(step==Step1)
    {
        if(currentSkin>=skinList.size())
            return;
        previous->setEnabled(currentSkin>0);
        next->setEnabled(currentSkin<(skinList.size()-1));
        paths.push_back(skinPath+skinList.at(currentSkin)+"/front.png");
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup>=monstergroup.size())
            return;
        previous->setEnabled(currentMonsterGroup>0);
        next->setEnabled(currentMonsterGroup<(monstergroup.size()-1));
        const std::vector<CatchChallenger::Profile::Monster> &monsters=monstergroup.at(currentMonsterGroup);
        unsigned int index=0;
        while(index<monsters.size())
        {
            const CatchChallenger::Profile::Monster &monster=monsters.at(index);
            paths.push_back(monsterPath+std::to_string(monster.id)+"/front.png");
            index++;
        }
    }
    else
        return;

    foreach (QGraphicsPixmapItem * item, centerImages)
        delete item;
    centerImages.clear();
    if(!paths.empty())
    {
        unsigned int index=0;
        while(index<paths.size())
        {
            const std::string &path=paths.at(index);

            QImage skin=QImage(QString::fromStdString(path));
            if(skin.isNull())
            {
                warning->setHtml(tr("But the skin can't be loaded: %1").arg(QString::fromStdString(path)));
                warning->setVisible(true);
                return;
            }
            QImage scaledSkin=skin.scaled(160,160,Qt::IgnoreAspectRatio);
            QPixmap pixmap;
            pixmap.convertFromImage(scaledSkin);
            QGraphicsPixmapItem * item=new QGraphicsPixmapItem(pixmap,this);
            centerImages.push_back(item);
            skinLoaded=true;

            index++;
        }
    }
    else
    {
        skinLoaded=false;
    }
}

bool NewGame::haveTheInformation()
{
    return okCanBeEnabled() && step==StepOk;
}

bool NewGame::okCanBeEnabled()
{
    return !uipseudo->text().isEmpty() && skinLoaded;
}

std::string NewGame::pseudo()
{
    return uipseudo->text().toStdString();
}

std::string NewGame::skin()
{
    return skinList.at(currentSkin);
}

uint8_t NewGame::skinId()
{
    return skinListId.at(currentSkin);
}

uint8_t NewGame::monsterGroupId()
{
    return currentMonsterGroup;
}

void NewGame::on_ok_clicked()
{
    if(step==Step1)
    {
        if(uipseudo->text().isEmpty())
        {
            warning->setHtml(tr("Your pseudo can't be empty"));
            warning->setVisible(true);
            return;
        }
        if(uipseudo->text().size()>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
        {
            warning->setHtml(tr("Your pseudo can't be greater than %1").arg(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
            warning->setVisible(true);
            return;
        }
        step=Step2;
        uipseudo->hide();
        updateSkin();
        if(monstergroup.size()<2)
            on_ok_clicked();
        if(uipseudo->text().contains(" "))
        {
            warning->setHtml(tr("Your pseudo can't contains space"));
            warning->setVisible(true);
            return;
        }
    }
    else if(step==Step2)
    {
        step=StepOk;
        ok=true;
        emit removeAbove();
    }
    else
        return;
}

void NewGame::on_pseudo_textChanged(const QString &)
{
    validate->setEnabled(okCanBeEnabled());
}

void NewGame::on_pseudo_returnPressed()
{
    on_ok_clicked();
}

void NewGame::on_next_clicked()
{
    if(step==Step1)
    {
        if(currentSkin<(skinList.size()-1))
            currentSkin++;
        else
            return;
        updateSkin();
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup<(monstergroup.size()-1))
            currentMonsterGroup++;
        else
            return;
        updateSkin();
    }
    else
        return;
}

void NewGame::on_previous_clicked()
{
    if(step==Step1)
    {
        if(currentSkin==0)
            return;
        currentSkin--;
        updateSkin();
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup==0)
            return;
        currentMonsterGroup--;
        updateSkin();
    }
    else
        return;
}

void NewGame::keyPressEvent(QKeyEvent * event)
{
    uipseudo->keyPressEvent(event);
}

void NewGame::keyReleaseEvent(QKeyEvent *event)
{
    uipseudo->keyReleaseEvent(event);
}
