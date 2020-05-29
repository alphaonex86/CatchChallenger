#include "AddCharacter.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Settings.hpp"
#include "../Language.hpp"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTextDocument>
#include "../../qt/Ultimate.hpp"
#include "../../../general/base/DatapackGeneralLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include <QDesktopServices>
#include <QDebug>

AddCharacter::AddCharacter() :
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
    connect(quit,&CustomButton::clicked,this,&AddCharacter::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    comboBox=new ComboBox(this);
    description=new QGraphicsTextItem(this);

    if(!connect(&Language::language,&Language::newLanguage,this,&AddCharacter::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&AddCharacter::on_cancel_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(validate,&CustomButton::clicked,this,&AddCharacter::on_ok_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(comboBox,static_cast<void(ComboBox::*)(int)>(&ComboBox::currentIndexChanged),this,&AddCharacter::on_comboBox_currentIndexChanged,Qt::QueuedConnection))
        abort();

    newLanguage();
}

AddCharacter::~AddCharacter()
{
    delete wdialog;
    delete quit;
    delete validate;
    delete title;
}

QRectF AddCharacter::boundingRect() const
{
    return QRectF();
}

void AddCharacter::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    auto font=description->font();
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
    description->setFont(font);

    unsigned int nameBackgroundNewHeight=50;
    unsigned int space=30;
    if(widget->width()<600 || widget->height()<480)
    {
        font.setPixelSize(30*0.75/2);
        space=10;
        nameBackgroundNewHeight=50/2;
    }
    else
    {
        font.setPixelSize(30*0.75);
    }

    quit->setPos(x+(idealW/2-quit->width()-space/2),y+idealH-quit->height()/2);
    validate->setPos(x+(idealW/2+space/2),y+idealH-quit->height()/2);
    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    /*const QRectF &serverTextRect=serverText->boundingRect();
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
    }*/
}

void AddCharacter::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    validate->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void AddCharacter::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    validate->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void AddCharacter::setDatapack(std::string path)
{
    this->datapackPath=path;
    this->ok=false;

    comboBox->clear();
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
    {
        description->setHtml(tr("No profile selected to start a new game"));
        return;
    }
    loadProfileText();
    unsigned int index=0;
    while(index<profileTextList.size())
    {
        comboBox->addItem(QString::fromStdString(profileTextList.at(index).name));
        index++;
    }
    if(comboBox->count()>0)
    {
        comboBox->setCurrentIndex(rand()%comboBox->count());
        description->setHtml(QString::fromStdString(profileTextList.at(comboBox->currentIndex()).description));

    }
    validate->setEnabled(comboBox->count()>0);
    if(profileTextList.size()==1)
        on_ok_clicked();
}

void AddCharacter::newLanguage()
{
    title->setText(tr("Select your profile"));
}

void AddCharacter::on_ok_clicked()
{
    if(comboBox->count()<1)
        return;
    ok=true;
    emit removeAbove();
}

bool AddCharacter::isOk() const
{
    return ok;
}

void AddCharacter::on_cancel_clicked()
{
    ok=false;
    emit removeAbove();
}

void AddCharacter::loadProfileText()
{
    const std::string &xmlFile=datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"start.xml";
    std::vector<const tinyxml2::XMLElement *> xmlList=CatchChallenger::DatapackGeneralLoader::loadProfileList(
                datapackPath,xmlFile,
                CatchChallenger::CommonDatapack::commonDatapack.items.item,
                CatchChallenger::CommonDatapack::commonDatapack.monsters,
                CatchChallenger::CommonDatapack::commonDatapack.reputation
                ).first;
    profileTextList.clear();
    unsigned int index=0;
    while(index<xmlList.size())
    {
        ProfileText profile;
        const tinyxml2::XMLElement * startItem=xmlList.at(index);
        #ifndef CATCHCHALLENGER_BOT
        const std::string &language=Language::language.getLanguage().toStdString();
        #else
        const std::string language("en");
        #endif
        bool found=false;
        const tinyxml2::XMLElement * name = startItem->FirstChildElement("name");
        if(!language.empty() && language!="en")
            while(name!=NULL)
            {
                if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                {
                    profile.name=name->GetText();
                    found=true;
                    break;
                }
                name = name->NextSiblingElement("name");
            }
        if(!found)
        {
            name = startItem->FirstChildElement("name");
            while(name!=NULL)
            {
                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                    if(name->GetText()!=NULL)
                    {
                        profile.name=name->GetText();
                        break;
                    }
                name = name->NextSiblingElement("name");
            }
        }
        if(profile.name.empty())
        {
            if(startItem->GetText()!=NULL)
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, name empty or not found: child.tagName(): %2")
                         .arg(QString::fromStdString(xmlFile)).arg(startItem->GetText()));
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, name empty or not found: child.tagName(")
                         .arg(QString::fromStdString(xmlFile)));
            startItem = startItem->NextSiblingElement("start");
            continue;
        }
        found=false;
        const tinyxml2::XMLElement * description = startItem->FirstChildElement("description");
        if(!language.empty() && language!="en")
            while(description!=NULL)
            {
                if(description->Attribute("lang")!=NULL && description->Attribute("lang")==language && description->GetText()!=NULL)
                {
                    profile.description=description->GetText();
                    found=true;
                    break;
                }
                description = description->NextSiblingElement("description");
            }
        if(!found)
        {
            description = startItem->FirstChildElement("description");
            while(description!=NULL)
            {
                if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                    if(description->GetText()!=NULL)
                    {
                        profile.description=description->GetText();
                        break;
                    }
                description = description->NextSiblingElement("description");
            }
        }
        if(profile.description.empty())
        {
            if(description->GetText()!=NULL)
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, description empty or not found: child.tagName(): %2")
                         .arg(QString::fromStdString(xmlFile)).arg(startItem->GetText()));
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, description empty or not found: child.tagName()")
                         .arg(QString::fromStdString(xmlFile)));
            startItem = startItem->NextSiblingElement("start");
            continue;
        }
        profileTextList.push_back(profile);
        index++;
    }
}

int AddCharacter::getProfileIndex()
{
    if(comboBox->count()>0)
        return comboBox->currentIndex();
    return 0;
}

int AddCharacter::getProfileCount()
{
    return comboBox->count();
}

void AddCharacter::on_comboBox_currentIndexChanged(int index)
{
    if(comboBox->count()>0)
        description->setHtml(QString::fromStdString(profileTextList.at(index).description));
}

