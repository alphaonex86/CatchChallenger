#include "NewGame.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Settings.hpp"
#include "../Language.hpp"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTextDocument>
#include "../../qt/Ultimate.hpp"
#include <QDesktopServices>

NewGame::NewGame() :
    wdialog(new CCWidget(this)),
    label(this)
{
    ok=false;
    edit=false;
    #if defined(NOTCPSOCKET) || defined(NOWEBSOCKET)
        ui->type->hide();
        #if defined(NOTCPSOCKET)
            ui->port->hide();
            ui->server->setPlaceholderText("ws://www.server.com:9999/");
        #endif
    #endif

    x=-1;
    y=-1;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",this);
    validate=new CustomButton(":/CC/images/interface/validate.png",this);
    connect(quit,&CustomButton::clicked,this,&NewGame::removeAbove);
    title=new CCDialogTitle(this);

    QPixmap p=*GameLoader::gameLoader->getImage(":/CC/images/interface/inputText.png");
    p=p.scaled(p.width(),50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    typeListProxy=new QGraphicsProxyWidget(this);
    m_type=new QComboBox();
    m_type->addItem(QString("Tcp"));
    m_type->addItem(QString("WS"));
    typeListProxy->setWidget(m_type);
    serverText=new QGraphicsTextItem(this);
    serverText->setVisible(false);
    serverInput=new LineEdit(this);
    portInput=new SpinBox(this);
    portInput->setMinimum(1);
    portInput->setMaximum(65535);

    nameText=new CCGraphicsTextItem(this);
    nameInput=new LineEdit(this);

    proxyText=new QGraphicsTextItem(this);
    proxyInput=new LineEdit(this);
    proxyPortInput=new SpinBox(this);
    proxyPortInput->setMinimum(1);
    proxyPortInput->setMaximum(65535);

    warning=new QGraphicsTextItem(this);
    warning->setVisible(false);

    if(!connect(&Language::language,&Language::newLanguage,this,&NewGame::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&NewGame::on_cancel_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(validate,&CustomButton::clicked,this,&NewGame::on_ok_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(m_type,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&NewGame::on_type_currentIndexChanged,Qt::QueuedConnection))
        abort();
    on_type_currentIndexChanged(0);

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

    auto font=serverText->font();
    if(widget->width()<600 || widget->height()<480)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        validate->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(30/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        validate->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(30);
    }
    serverText->setFont(font);
    nameText->setFont(font);
    proxyText->setFont(font);

    unsigned int nameBackgroundNewHeight=50;
    unsigned int space=30;
    if(widget->width()<600 || widget->height()<480)
    {
        font.setPixelSize(30*0.75/2);
        serverInput->setFont(font);
        portInput->setFont(font);
        nameInput->setFont(font);
        proxyInput->setFont(font);
        proxyPortInput->setFont(font);
        space=10;
        nameBackgroundNewHeight=50/2;
    }
    else
    {
        font.setPixelSize(30*0.75);
        serverInput->setFont(font);
        portInput->setFont(font);
        nameInput->setFont(font);
        proxyInput->setFont(font);
        proxyPortInput->setFont(font);
    }

    quit->setPos(x+(idealW/2-quit->width()-space/2),y+idealH-quit->height()/2);
    validate->setPos(x+(idealW/2+space/2),y+idealH-quit->height()/2);
    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    const QRectF &serverTextRect=serverText->boundingRect();
    const QRectF &nameTextRect=nameText->boundingRect();
    const QRectF &proxyTextRect=proxyText->boundingRect();

    const unsigned int &serverBackgroundNewWidth=idealW-nameTextRect.width()-wdialog->currentBorderSize()*4;
    const unsigned int &nameBackgroundNewWidth=idealW-nameTextRect.width()-wdialog->currentBorderSize()*4;
    const unsigned int &proxyBackgroundNewWidth=idealW-nameTextRect.width()-wdialog->currentBorderSize()*4;
    if((unsigned int)nameInput->width()!=nameBackgroundNewWidth ||
            (unsigned int)nameInput->height()!=nameBackgroundNewHeight)
    {
        serverInput->setFixedHeight(nameBackgroundNewHeight);
        portInput->setFixedHeight(nameBackgroundNewHeight);
        if(portInput->isVisible())
        {
            serverInput->setFixedWidth(serverBackgroundNewWidth*3/4);
            portInput->setFixedWidth(serverBackgroundNewWidth*1/4);
        }
        else
            serverInput->setFixedWidth(serverBackgroundNewWidth);
        nameInput->setFixedSize(nameBackgroundNewWidth,nameBackgroundNewHeight);
        proxyInput->setFixedSize(proxyBackgroundNewWidth*3/4,nameBackgroundNewHeight);
        proxyPortInput->setFixedSize(proxyBackgroundNewWidth*1/4,nameBackgroundNewHeight);
    }
    {
        typeListProxy->setPos(x+wdialog->currentBorderSize()*2,y+top*1.5);
        serverText->setPos(x+wdialog->currentBorderSize()*2,y+top*1.5);
        const unsigned int serverBackgroundW=serverText->x()+serverTextRect.width();
        serverInput->setPos(serverBackgroundW,serverText->y()+(serverTextRect.height()-serverInput->boundingRect().height())/2);
        if(portInput->isVisible())
            portInput->setPos(serverBackgroundW+serverInput->width(),serverText->y()+(serverTextRect.height()-serverInput->boundingRect().height())/2);
    }
    {
        nameText->setPos(x+wdialog->currentBorderSize()*2,serverText->y()+serverTextRect.height()+space);
        const unsigned int nameBackgroundW=nameText->x()+nameTextRect.width();
        nameInput->setPos(nameBackgroundW,nameText->y()+(nameTextRect.height()-nameInput->boundingRect().height())/2);
    }
    {
        proxyText->setPos(x+wdialog->currentBorderSize()*2,nameText->y()+nameTextRect.height()+space);
        const unsigned int proxyBackgroundW=proxyText->x()+proxyTextRect.width();
        proxyInput->setPos(proxyBackgroundW,proxyText->y()+(proxyTextRect.height()-proxyInput->boundingRect().height())/2);
        proxyPortInput->setPos(proxyBackgroundW+proxyInput->width(),proxyText->y()+(proxyTextRect.height()-proxyInput->boundingRect().height())/2);
    }
}

void NewGame::mousePressEventXY(const QPointF &p, bool &pressValidated)
{
    quit->mousePressEventXY(p,pressValidated);
    validate->mousePressEventXY(p,pressValidated);
}

void NewGame::mouseReleaseEventXY(const QPointF &p,bool &pressValidated)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    validate->mouseReleaseEventXY(p,pressValidated);
}

void NewGame::mouseMoveEventXY(const QPointF &p,bool &pressValidated)
{
    Q_UNUSED(p);
    Q_UNUSED(pressValidated);
}

void NewGame::setDatapack(const std::string &skinPath, const std::string &monsterPath, std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup, const std::vector<uint8_t> &forcedSkin)
{

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
    proxyText->setHtml(tr("Proxy: "));
    nameText->setHtml(tr("Name: "));
    serverText->setHtml(tr("Server: "));
    if(edit)
        title->setText(tr("Edit"));
    else
        title->setText(tr("Add"));
}

int NewGame::type() const
{
#if ! defined(NOTCPSOCKET) && ! defined(NOWEBSOCKET)
return m_type->currentIndex();
#else
    #if defined(NOTCPSOCKET)
    return 1;
    #else
        #if defined(NOWEBSOCKET)
        return 0;
        #else
        #error add server but no tcp or web socket defined
        return -1;
        #endif
    #endif
#endif
}

void NewGame::setType(const int &type)
{
#if ! defined(NOTCPSOCKET) && ! defined(NOWEBSOCKET)
m_type->setCurrentIndex(type);
#else
    #if defined(NOTCPSOCKET)
    ui->type->setCurrentIndex(1);
    #else
        #if defined(NOWEBSOCKET)
        ui->type->setCurrentIndex(0);
        #endif
    #endif
#endif
        Q_UNUSED(type);
}

void NewGame::setEdit(const bool &edit)
{
    this->edit=edit;
    title->setText(tr("Edit"));
}

void NewGame::on_ok_clicked()
{
    if(nameText->toPlainText()==QStringLiteral("Internal") || nameText->toPlainText()==QStringLiteral("internal"))
    {
        warning->setHtml(tr("The name can't be \"internal\""));
        warning->setVisible(true);
        return;
    }
    if(type()==0)
    {
        if(!server().contains(QRegularExpression("^[a-zA-Z0-9\\.:\\-_]+$")))
        {
            warning->setHtml(tr("The host seam don't be a valid hostname or ip"));
            warning->setVisible(true);
            return;
        }
    }
    else if(type()==1)
    {
        if(!server().startsWith("ws://") && !server().startsWith("wss://"))
        {
            warning->setHtml(tr("The web socket url seam wrong, not start with ws:// or wss://"));
            warning->setVisible(true);
            return;
        }
    }
    ok=true;
    emit quitOption();
    //close();
}

QString NewGame::server() const
{
    return serverInput->text();
}

uint16_t NewGame::port() const
{
    return static_cast<uint16_t>(portInput->value());
}

QString NewGame::proxyServer() const
{
    return proxyInput->text();
}

uint16_t NewGame::proxyPort() const
{
    return static_cast<uint16_t>(proxyPortInput->value());
}

QString NewGame::name() const
{
    return nameInput->text();
}

void NewGame::setServer(const QString &server)
{
    serverInput->setText(server);
}

void NewGame::setPort(const uint16_t &port)
{
    portInput->setValue(port);
}

void NewGame::setName(const QString &name)
{
    nameInput->setText(name);
}

void NewGame::setProxyServer(const QString &proxyServer)
{
    proxyInput->setText(proxyServer);
}

void NewGame::setProxyPort(const uint16_t &proxyPort)
{
    proxyPortInput->setValue(proxyPort);
}

bool NewGame::isOk() const
{
    return ok;
}

bool NewGame::haveSkin() const
{

}

void NewGame::on_type_currentIndexChanged(int index)
{
    switch(index) {
    case 0:
        portInput->show();
        serverInput->setPlaceholderText("www.server.com");
        break;
    case 1:
        portInput->hide();
        serverInput->setPlaceholderText("ws://www.server.com:9999/");
        break;
    default:
        return;
    }
}

void NewGame::on_cancel_clicked()
{
    ok=false;
    emit quitOption();
}

NewGame::NewGame(const std::string &skinPath, const std::string &monsterPath, std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup, const std::vector<uint8_t> &forcedSkin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGame)
{
    srand(static_cast<uint32_t>(time(NULL)));
    ui->setupUi(this);
    this->forcedSkin=forcedSkin;
    this->monsterPath=monsterPath;
    this->monstergroup=monstergroup;
    okAccepted=true;
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

    ui->pseudo->setMaxLength(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size);
    ui->previousSkin->setVisible(skinList.size()>=2);
    ui->nextSkin->setVisible(skinList.size()>=2);

    currentSkin=0;
    if(!skinList.empty())
        currentSkin=static_cast<uint8_t>(rand()%skinList.size());
    updateSkin();
    ui->pseudo->setFocus();
    if(skinList.empty())
    {
        QMessageBox::critical(this,tr("Error"),tr("No skin to select!"));
        close();
        return;
    }

    if(!connect(&timer,&QTimer::timeout,      this,&NewGame::timerSlot,    Qt::QueuedConnection))
        abort();
}

NewGame::~NewGame()
{
    delete ui;
}

void NewGame::updateSkin()
{
    skinLoaded=false;

    std::vector<std::string> paths;
    if(step==Step1)
    {
        if(currentSkin>=skinList.size())
            return;
        ui->previousSkin->setEnabled(currentSkin>0);
        ui->nextSkin->setEnabled(currentSkin<(skinList.size()-1));
        paths.push_back(skinPath+skinList.at(currentSkin)+"/front.png");
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup>=monstergroup.size())
            return;
        ui->previousSkin->setEnabled(currentMonsterGroup>0);
        ui->nextSkin->setEnabled(currentMonsterGroup<(monstergroup.size()-1));
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

    {
        QLayoutItem *child;
        while ((child = ui->horizontalLayout->takeAt(0)) != 0)
        {
            delete child->widget();
            delete child;
        }
    }
    if(!paths.empty())
    {
        unsigned int index=0;
        while(index<paths.size())
        {
            const std::string &path=paths.at(index);

            QImage skin=QImage(QString::fromStdString(path));
            if(skin.isNull())
            {
                QMessageBox::critical(this,tr("Error"),QStringLiteral("But the skin can't be loaded: %1").arg(QString::fromStdString(path)));
                return;
            }
            QImage scaledSkin=skin.scaled(160,160,Qt::IgnoreAspectRatio);
            QPixmap pixmap;
            pixmap.convertFromImage(scaledSkin);
            QLabel *label=new QLabel();
            label->setMinimumSize(160,160);
            label->setMaximumSize(160,160);
            ui->horizontalLayout->addWidget(label);
            label->setPixmap(pixmap);
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
    return !ui->pseudo->text().isEmpty() && skinLoaded;
}

std::string NewGame::pseudo()
{
    return ui->pseudo->text().toStdString();
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

bool NewGame::haveSkin()
{
    return skinList.size()>0;
}

void NewGame::on_ok_clicked()
{
    if(!okAccepted)
        return;
    okAccepted=false;
    timer.start(20);
    if(step==Step1)
    {
        if(ui->pseudo->text().isEmpty())
        {
            QMessageBox::warning(this,tr("Error"),tr("Your pseudo can't be empty"));
            return;
        }
        step=Step2;
        ui->pseudo->hide();
        updateSkin();
        if(monstergroup.size()<2)
            on_ok_clicked();
        if(characterEntry.pseudo.find(" ")!=std::string::npos)
        {
            QMessageBox::warning(this,tr("Error"),tr("Your psuedo can't contains space"));
            connexionManager->client->tryDisconnect();
            return;
        }
    }
    else if(step==Step2)
    {
        step=StepOk;
        accept();
    }
    else
        return;
}

void NewGame::on_pseudo_textChanged(const QString &)
{
    ui->ok->setEnabled(okCanBeEnabled());
}

void NewGame::on_pseudo_returnPressed()
{
    on_ok_clicked();
}

void NewGame::on_nextSkin_clicked()
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

void NewGame::on_previousSkin_clicked()
{
    if(step==Step1)
    {
        if(currentSkin<=0)
            return;
        currentSkin--;
        updateSkin();
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup<=0)
            return;
        currentMonsterGroup--;
        updateSkin();
    }
    else
        return;
}

void NewGame::timerSlot()
{
    okAccepted=true;
}

