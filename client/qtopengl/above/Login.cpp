#include "Login.hpp"
#include "../../qt/GameLoader.hpp"
#include "../Language.hpp"
#include "../../qt/BlacklistPassword.hpp"
#include <QPainter>

Login::Login() :
    wdialog(new CCWidget(this)),
    label(this)
{
    validated=false;
    x=-1;
    y=-1;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    title=new CCDialogTitle(this);

    warning=new QGraphicsTextItem(this);
    loginText=new QGraphicsTextItem(this);
    login=new LineEdit(this);
    passwordText=new QGraphicsTextItem(this);
    password=new LineEdit(this);
    password->setEchoMode(QLineEdit::Password);
    remember=new CheckBox(this);
    rememberText=new QGraphicsTextItem(this);
    htmlText=new QGraphicsTextItem(this);
    htmlText->setOpenExternalLinks(true);
    //htmlText->setCacheMode(QGraphicsItem::DeviceCoordinateCache);->bug
    //htmlText->setCacheMode(QGraphicsItem::ItemCoordinateCache);->spam console
    htmlText->setTextInteractionFlags(Qt::TextBrowserInteraction);

    server_select=new CustomButton(":/CC/images/interface/validate.png",this);
    back=new CustomButton(":/CC/images/interface/cancel.png",this);

    if(!connect(server_select,&CustomButton::clicked,this,&Login::validate))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Login::quitLogin))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&Login::newLanguage,Qt::QueuedConnection))
        abort();
    newLanguage();
}

Login::~Login()
{
}

void Login::setAuth(const QStringList &list)
{
    m_auth=list;
    if(list.size()<3)
        return;
    QString n=list.first();
    int i=n.toUInt();
    if(i<(list.size()-1)/2)
    {
        login->setText(list.at(1+i*2));
        password->setText(list.at(1+i*2+1));
        remember->setChecked(!password->text().isEmpty());
    }
}

void Login::setLinks(const QString &site_page,const QString &register_page)
{
    QStringList links;
    if(!site_page.isEmpty())
        links.append("<a href=\""+site_page+"\">"+tr("Web site")+"</a>");
    if(!register_page.isEmpty())
        links.append("<a href=\""+register_page+"\">"+tr("Register")+"</a>");
    htmlText->setHtml(links.join("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
}

QStringList Login::getAuth()
{
    QString l=login->text();
    QString p=password->text();
    QStringList list=m_auth;
    int index=1;
    while(index<list.size())
    {
        if(list.at(index)==l)
        {
            if(remember->isChecked())
                list[index+1]=p;
            else
                list[index+1]=QString();
            list.first()=QString::number((index-1)/2);
            break;
        }
        index+=2;
    }
    if(index>=list.size())
    {
        QString Si=QString::number((list.size()-1)/2-1);
        if(!list.isEmpty())
            list[0]=Si;
        else
            list.append(Si);
        list.append(l);
        list.append(p);
    }
    if(m_auth!=list)
        return list;
    else
        return QStringList();
}

QString Login::getLogin()
{
    return login->text();
}

QString Login::getPass()
{
    return password->text();
}

bool Login::isOk()
{
    return validated;
}

QRectF Login::boundingRect() const
{
    return QRectF();
}

void Login::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    bool forcedCenter=true;
    int idealW=900;
    int space=10;
    int idealH=40+space+40+space+40+space+40+150;
    if(widget->width()<600 || widget->height()<640)
    {
        idealW/=2;
        idealH=idealH/2+20;
        space/=2;
    }
    if(idealW>widget->width())
        idealW=widget->width();
    int top=36;
    int bottom=94/2;
    if(widget->width()<600 || widget->height()<640)
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

    auto font=loginText->font();
    if(widget->width()<600 || widget->height()<640)
    {
        label.setScale(0.5);
        back->setSize(83/2,94/2);
        server_select->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(30/2);
    }
    else {
        label.setScale(1);
        back->setSize(83,94);
        server_select->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(30);
    }
    loginText->setFont(font);
    passwordText->setFont(font);
    rememberText->setFont(font);
    htmlText->setFont(font);
    warning->setFont(font);

    unsigned int productKeyBackgroundNewHeight=50;
    if(widget->width()<600 || widget->height()<640)
    {
        font.setPixelSize(30*0.75/2);
        login->setFont(font);
        password->setFont(font);
        productKeyBackgroundNewHeight=50/2;
    }
    else
    {
        font.setPixelSize(30*0.75);
        login->setFont(font);
        password->setFont(font);
    }

    const QRectF &loginTextRect=loginText->boundingRect();
    const QRectF &productKeyTextRect=passwordText->boundingRect();
    int maxWidth=loginTextRect.width();
    if(maxWidth<productKeyTextRect.width())
        maxWidth=productKeyTextRect.width();

    const unsigned int &productKeyBackgroundNewWidth=idealW-maxWidth-wdialog->currentBorderSize()*4;
    login->setMaximumSize(productKeyBackgroundNewWidth,productKeyBackgroundNewHeight);
    login->setMinimumSize(productKeyBackgroundNewWidth,productKeyBackgroundNewHeight);
    password->setMaximumSize(productKeyBackgroundNewWidth,productKeyBackgroundNewHeight);
    password->setMinimumSize(productKeyBackgroundNewWidth,productKeyBackgroundNewHeight);

    back->setPos(x+(idealW/2-back->width()-space/2),y+idealH-back->height()/2);
    server_select->setPos(x+(idealW/2+space/2),y+idealH-back->height()/2);
    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    {
        loginText->setPos(x+wdialog->currentBorderSize()*2,y+top*1.5);
        const unsigned int volumeSliderW=loginText->x()+loginTextRect.width();
        login->setPos(volumeSliderW,loginText->y()+(loginTextRect.height()-login->boundingRect().height())/2);
        login->setFixedWidth(idealW-loginTextRect.width()-wdialog->currentBorderSize()*4);
    }
    {
        passwordText->setPos(x+wdialog->currentBorderSize()*2,loginText->y()+loginTextRect.height()+space);
        const unsigned int productKeyBackgroundW=passwordText->x()+productKeyTextRect.width();
        password->setPos(productKeyBackgroundW,passwordText->y()+(productKeyTextRect.height()-passwordText->boundingRect().height())/2);
    }
    {
        rememberText->setPos(x+wdialog->currentBorderSize()*2+remember->boundingRect().width()+space,
                             passwordText->y()+passwordText->boundingRect().height()+space);
        remember->setPos(x+wdialog->currentBorderSize()*2,rememberText->y()+rememberText->boundingRect().height()/2-remember->boundingRect().height()/2);
    }
    {
        htmlText->setPos(x+wdialog->currentBorderSize()*2,rememberText->y()+rememberText->boundingRect().height());
    }
    warning->setPos(widget->width()/2-warning->boundingRect().width()/2,space);
}

void Login::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    back->mousePressEventXY(p,pressValidated);
    server_select->mousePressEventXY(p,pressValidated);
}

void Login::mouseReleaseEventXY(const QPointF &p, bool &pressValidated)
{
    back->mouseReleaseEventXY(p,pressValidated);
    server_select->mouseReleaseEventXY(p,pressValidated);
}

void Login::validate()
{
    if(password->text().size()<6)
    {
        warning->setHtml("<dev style=\"background-color:#fcc;\">"+tr("Your password need to be at minimum of 6 characters")+"</dev>");
        warning->setVisible(true);
        return;
    }
    {
        std::string pass=password->text().toStdString();
        std::transform(pass.begin(), pass.end(), pass.begin(), ::tolower);
        unsigned int index=0;
        while(index<BlacklistPassword::list.size())
        {
            if(BlacklistPassword::list.at(index)==pass)
            {
                warning->setHtml("<dev style=\"background-color:#fcc;\">"+tr("Your password is into the most common password in the world, too easy to crack dude! Change it!")+"</dev>");
                warning->setVisible(true);
                return;
            }
            index++;
        }
    }
    if(login->text().size()<3)
    {
        warning->setHtml("<dev style=\"background-color:#fcc;\">"+tr("Your login need to be at minimum of 3 characters")+"</dev>");
        warning->setVisible(true);
        return;
    }
    if(password->text()==login->text())
    {
        warning->setHtml("<dev style=\"background-color:#fcc;\">"+tr("Your login can't be same as your login")+"</dev>");
        warning->setVisible(true);
        return;
    }

    validated=true;
    quitLogin();
}

void Login::newLanguage()
{
    //warning->setHtml();
    loginText->setHtml(tr("Login: "));
    passwordText->setHtml(tr("Password: "));
    rememberText->setHtml(tr("Remember the password"));
    title->setText(tr("Login"));
    //htmlText->setHtml(tr(""));
}
