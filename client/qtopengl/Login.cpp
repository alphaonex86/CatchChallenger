#include "Login.hpp"

Login::Login() :
    /*addServer(nullptr),
    login(nullptr),*/
    reply(nullptr)
{
    srand(time(0));

    temp_xmlConnexionInfoList=loadXmlConnexionInfoList();
    temp_customConnexionInfoList=loadConfigConnexionInfoList();
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());
    selectedServer.unique_code.clear();
    selectedServer.isCustom=false;

    server_add=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    server_add->setOutlineColor(QColor(44,117,0));
    server_remove=new CustomButton(":/CC/images/interface/redbutton.png",this);
    server_remove->setOutlineColor(QColor(125,0,0));
    server_edit=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    server_edit->setOutlineColor(QColor(44,117,0));
    server_refresh=new CustomButton(":/CC/images/interface/refresh.png",this);

    server_select=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    wdialog=new CCWidget(this);
    warning=new QGraphicsTextItem(this);
    serverEmpty=new QGraphicsTextItem(this);
    scrollZone=new CCScrollZone(this);

    if(!connect(server_add,&CustomButton::clicked,this,&Login::server_add_clicked))
        abort();
    if(!connect(server_remove,&CustomButton::clicked,this,&Login::server_remove_clicked))
        abort();
    if(!connect(server_edit,&CustomButton::clicked,this,&Login::server_edit_clicked))
        abort();
    if(!connect(server_select,&CustomButton::clicked,this,&Login::server_select_clicked))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Login::backMain))
        abort();
    if(!connect(server_refresh,&CustomButton::clicked,this,&Login::on_server_refresh_clicked))
        abort();
    newLanguage();

    //need be the last
    downloadFile();
    displayServerList();
    addServer=nullptr;
}

Login::~Multi()
{
}

void Login::newLanguage()
{
    server_add->setText(tr("Add"));
    server_remove->setText(tr("Remove"));
    server_edit->setText(tr("Edit"));
    warning->setHtml("<span style=\"background-color: rgb(255, 255, 163);\nborder: 1px solid rgb(255, 221, 50);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\">"+tr("Loading the servers list...")+"</span>");
    serverEmpty->setHtml(QStringLiteral("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
}

QRectF Login::boundingRect() const
{
    return QRectF();
}

void Login::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
{
    unsigned int space=10;
    unsigned int fontSize=20;
    unsigned int multiItemH=100;
    if(w->height()>300)
        fontSize=w->height()/6;
    if(w->width()<600 || w->height()<600)
    {
        server_add->setSize(148,61);
        server_add->setPixelSize(23);
        server_remove->setSize(148,61);
        server_remove->setPixelSize(23);
        server_edit->setSize(148,61);
        server_edit->setPixelSize(23);
        server_refresh->setSize(56,62);
        server_select->setSize(56,62);
        back->setSize(56,62);
        multiItemH=50;
    }
    else if(w->width()<900 || w->height()<600)
    {
        server_add->setSize(148,61);
        server_add->setPixelSize(23);
        server_remove->setSize(148,61);
        server_remove->setPixelSize(23);
        server_edit->setSize(148,61);
        server_edit->setPixelSize(23);
        server_refresh->setSize(56,62);
        server_select->setSize(84,93);
        back->setSize(84,93);
        multiItemH=75;
    }
    else {
        space=30;
        server_add->setSize(223,92);
        server_add->setPixelSize(35);
        server_remove->setSize(224,92);
        server_remove->setPixelSize(35);
        server_edit->setSize(223,92);
        server_edit->setPixelSize(35);
        server_refresh->setSize(84,93);
        server_select->setSize(84,93);
        back->setSize(84,93);
    }


    unsigned int tWidthTButton=server_add->width()+space+
            server_edit->width()+space+
            server_remove->width()+space+
            server_refresh->width();
    unsigned int tXTButton=w->width()/2-tWidthTButton/2;
    unsigned int tWidthTButtonOffset=0;
    unsigned int y=w->height()-space-server_select->height()-space-server_add->height();
    server_add->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_add->width()+space;
    server_edit->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_edit->width()+space;
    server_remove->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_remove->width()+space;
    server_refresh->setPos(tXTButton+tWidthTButtonOffset,y);

    tWidthTButton=server_select->width()+space+
            back->width();
    y=w->height()-space-server_select->height();
    tXTButton=w->width()/2-tWidthTButton/2;
    back->setPos(tXTButton,y);
    server_select->setPos(tXTButton+back->width()+space,y);

    y=w->height()-space-server_select->height()-space-server_add->height();
    wdialog->setHeight(w->height()-space-server_select->height()-space-server_add->height()-space-space);
    if((unsigned int)w->width()<(800+space*2))
    {
        wdialog->setWidth(w->width()-space*2);
        wdialog->setPos(space,space);
    }
    else
    {
        wdialog->setWidth(800);
        wdialog->setPos(w->width()/2-wdialog->width()/2,space);
    }
    warning->setPos(w->width()/2-warning->boundingRect().width(),space+wdialog->height()-wdialog->currentBorderSize()-warning->boundingRect().height());

    unsigned int offsetMultiItem=0;
    foreach(MultiItem *item, serverConnexion)
    {
        item->setPos(wdialog->x()+wdialog->currentBorderSize(),space+wdialog->currentBorderSize()+offsetMultiItem);
        item->setSize(wdialog->width()-wdialog->currentBorderSize()*2,multiItemH);
        offsetMultiItem+=multiItemH+space/3;
    }
}

void Login::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    server_add->mousePressEventXY(p,pressValidated);
    server_remove->mousePressEventXY(p,pressValidated);
    server_edit->mousePressEventXY(p,pressValidated);
    server_refresh->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    server_select->mousePressEventXY(p,pressValidated);
    foreach(MultiItem *item, serverConnexion)
        item->mousePressEventXY(p,pressValidated);
}

void Login::mouseReleaseEventXY(const QPointF &p, bool &pressValidated)
{
    server_add->mouseReleaseEventXY(p,pressValidated);
    server_remove->mouseReleaseEventXY(p,pressValidated);
    server_edit->mouseReleaseEventXY(p,pressValidated);
    server_refresh->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    server_select->mouseReleaseEventXY(p,pressValidated);
    MultiItem *newSelectedItem=nullptr;
    foreach(MultiItem *item, serverConnexion)
    {
        const bool wasSelected=item->isSelected();
        item->mouseReleaseEventXY(p,pressValidated);
        if(wasSelected==false && newSelectedItem==nullptr && item->isSelected())
        {
            newSelectedItem=item;
            const ConnexionInfo &info=item->connexionInfo();
            selectedServer.unique_code=info.unique_code;
            selectedServer.isCustom=info.isCustom;
            server_edit->setEnabled(info.isCustom);
            server_remove->setEnabled(info.isCustom);
        }
    }
    server_select->setEnabled(!selectedServer.unique_code.isEmpty());
    if(newSelectedItem!=nullptr)
        foreach(MultiItem *item, serverConnexion)
            if(newSelectedItem!=item)
                item->setSelected(false);
}
