#include "CCBackground.h"
#include <QPainter>
#include <QTime>
#include <QTextItem>
#include <chrono>
#include <iostream>
#include <string>

CCBackground::CCBackground()
{
    floorMove=0;
    zoom=0;
    grassMove=0;
    treebackMove=0;
    treefrontMove=0;
    cloudMove=0;
    if(!QObject::connect(&grassTimer,&QTimer::timeout,this,&CCBackground::grassSlot))
        abort();
    if(!QObject::connect(&treebackTimer,&QTimer::timeout,this,&CCBackground::treebackSlot))
        abort();
    if(!QObject::connect(&treefrontTimer,&QTimer::timeout,this,&CCBackground::treefrontSlot))
        abort();
    if(!QObject::connect(&cloudTimer,&QTimer::timeout,this,&CCBackground::cloudSlot))
        abort();
    if(!QObject::connect(&updateTimer,&QTimer::timeout,this,&CCBackground::updateSlot))
        abort();
    startAnimation();
    //timeFromStart=std::chrono::steady_clock::now();
    player1Frame=0;
    animationStep=0;

    pauseBeforeDisplayDialog.setInterval(1000);
    pauseBeforeDisplayDialog.setSingleShot(true);
    pauseBeforeRemovePerSyllableDialog.setInterval(1000);
    pauseBeforeRemovePerSyllableDialog.setSingleShot(true);
    if(!QObject::connect(&pauseBeforeDisplayDialog,&QTimer::timeout,this,&CCBackground::showDialogSlot))
        abort();
    if(!QObject::connect(&pauseBeforeRemovePerSyllableDialog,&QTimer::timeout,this,&CCBackground::hideDialogSlot))
        abort();
    dialog1Text="";
    dialog2Text="";

    dialogs.push_back("2: Hi, what we doing here?");
    dialogs.push_back("1: It's the background of CatchChallenger!<br>What better place to speak about CatchChallenger?");
    dialogs.push_back("1: We will speak about technique, concept, news and events!");
}

int CCBackground::count_partial_letter(std::string s)
{
    int count = 0;
    bool previouslettercounted=false;
    for (int i = 0; i < s.size(); i++)
    {
        switch(s[i])
        {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
            if(!previouslettercounted)
            {
            count++;
            previouslettercounted=true;
            }
        break;
        default:
            previouslettercounted=false;
        break;
        }
    }
    return count;
}

void CCBackground::showDialogSlot()
{
    if(dialogs.empty())
        return;
    const std::string str=dialogs.at(0);
    dialogs.erase(dialogs.cbegin());
    const std::string finalstr=str.substr(3);
    if(str.at(0)=='1')
    {
        std::cout << "1: " << finalstr << std::endl;
        dialog1Text=finalstr;
        dialog1Qt.setHtml(QString::fromStdString(finalstr));
    }
    else
    {
        std::cout << "2: " << finalstr << std::endl;
        dialog2Text=finalstr;
        dialog2Qt.setHtml(QString::fromStdString(finalstr));
    }
    //compute time to show based on Syllable
    pauseBeforeRemovePerSyllableDialog.start(1000*count_partial_letter(finalstr));
}

void CCBackground::hideDialogSlot()
{
    dialog1Text="";
    dialog2Text="";
    dialog1Qt.setHtml("");
    dialog2Qt.setHtml("");
    if(!pauseBeforeDisplayDialog.isActive())
        pauseBeforeDisplayDialog.start();
}

QRectF CCBackground::boundingRect() const
{
    return QRectF();
}

void CCBackground::startAnimation()
{
    unsigned int baseTime=20;
    grassTimer.start(baseTime);
    treefrontTimer.start(baseTime*3);
    treebackTimer.start(baseTime*9);
    cloudTimer.start(baseTime*30);
    updateTimer.start(40);
}

void CCBackground::stopAnimation()
{
    grassTimer.stop();
    treefrontTimer.stop();
    treebackTimer.stop();
    cloudTimer.stop();
    updateTimer.stop();
}

void CCBackground::scalePix(QPixmap &pix,unsigned int zoom)
{
    pix=pix.scaled(pix.width()*zoom,pix.height()*zoom);
}

void CCBackground::updateSlot()
{
    update();
}

void CCBackground::grassSlot()
{
    if(grass.isNull())
        return;
    grassMove++;
    floorMove++;
}

void CCBackground::treebackSlot()
{
    if(treeback.isNull())
        return;
    treebackMove++;
}

void CCBackground::treefrontSlot()
{
    if(treefront.isNull())
        return;
    treefrontMove++;
}

void CCBackground::cloudSlot()
{
    if(cloud.isNull())
        return;
    cloudMove++;
}

unsigned int CCBackground::getTargetZoom(QWidget *widget)
{
    unsigned int targetZoomHeight=(unsigned int)widget->height()/120;
    if(targetZoomHeight<1)
        targetZoomHeight=1;
    if(targetZoomHeight>8)
        targetZoomHeight=8;
    unsigned int targetZoomWidth=(unsigned int)widget->width()/120;
    if(targetZoomWidth<1)
        targetZoomWidth=1;
    if(targetZoomWidth>8)
        targetZoomWidth=8;
    unsigned int targetZoom=targetZoomHeight;
    if(targetZoomWidth<targetZoomHeight)
        targetZoom=targetZoomWidth;
    return targetZoom;
}

void CCBackground::paint(QPainter *painter, const QStyleOptionGraphicsItem * option, QWidget *widget)
{
    QTime time;
    time.restart();
    unsigned int targetZoom=getTargetZoom(widget);
    if(zoom!=targetZoom)
    {
        //std::cout << "new zoom: " << targetZoom << " with " << width() << "*" << height() << std::endl;
        cloud=QPixmap(":/cloud.png");
        if(cloud.isNull())
            abort();
        grass=QPixmap(":/grass.png");
        if(grass.isNull())
            abort();
        sun=QPixmap(":/sun.png");
        if(sun.isNull())
            abort();
        treeback=QPixmap(":/treeback.png");
        if(treeback.isNull())
            abort();
        treefront=QPixmap(":/treefront.png");
        if(treefront.isNull())
            abort();

        player1=QPixmap(":/copyright/1.png");
        if(player1.isNull())
            abort();
        player2=QPixmap(":/copyright/1b.png");
        if(player2.isNull())
            abort();
        dialog1=QPixmap(":/copyright/dialog1.png");
        if(dialog1.isNull())
            abort();
        dialog2=QPixmap(":/copyright/dialog2.png");
        if(dialog2.isNull())
            abort();
        if(targetZoom>1)
        {
            scalePix(cloud,targetZoom);
            scalePix(grass,targetZoom);
            scalePix(sun,targetZoom);
            scalePix(treeback,targetZoom);
            scalePix(treefront,targetZoom);

            scalePix(player1,targetZoom);
            scalePix(player2,targetZoom);
        }
        grassMove=rand()%grass.width()*targetZoom;
        treebackMove=rand()%treeback.width()*targetZoom;
        treefrontMove=rand()%treefront.width()*targetZoom;
        cloudMove=rand()%cloud.width();
        zoom=targetZoom;
    }
    treefrontMove%=treefront.width()*targetZoom;
    cloudMove%=cloud.width()*targetZoom;
    treebackMove%=treeback.width()*targetZoom;
    grassMove%=grass.width()*targetZoom;
    //paint.fillRect(rect(),QColor(116,225,255));

    int grassOffset=0;
    int skyOffset=0;
    int sunOffset=0;
    unsigned int endOfGrass=0;
    if((unsigned int)widget->height()<=120*targetZoom)
        endOfGrass=widget->height();
    else
    {
        skyOffset=(widget->height()-120*targetZoom)*2/3/*66%*/;
        if(skyOffset>widget->height()/4)
            skyOffset=widget->height()/4;
        endOfGrass=120*targetZoom+(widget->height()-120*targetZoom)*2/3/*66%*/;
        painter->fillRect(0,endOfGrass,widget->width(),widget->height()-endOfGrass,QColor(131,203,83));
        painter->fillRect(0,0,widget->width(),skyOffset,QColor(115,225,255));
    }
    sunOffset=endOfGrass/6/*16%*/;

    QRect gradientRect(0,skyOffset,widget->width(),120*targetZoom);
    QLinearGradient gradient(gradientRect.topLeft(), gradientRect.bottomLeft()); // diagonal gradient from top-left to bottom-right
    gradient.setColorAt(0, QColor(115,225,255));
    gradient.setColorAt(1, QColor(183,241,255));
    painter->fillRect(0,skyOffset,widget->width(),endOfGrass-skyOffset,gradient);

    grassOffset=endOfGrass-grass.height();
    unsigned int treeFrontOffset=grassOffset+4*targetZoom-treefront.height();
    unsigned int treebackOffset=grassOffset+4*targetZoom-treeback.height();

    int lastTreebackX=-treebackMove;
    while(widget->width()>lastTreebackX)
    {
        painter->drawPixmap(lastTreebackX,treebackOffset,treeback.width(),    treeback.height(),    treeback);
        lastTreebackX+=treeback.width();
    }
    int lastTreefrontX=-treefrontMove;
    while(widget->width()>lastTreefrontX)
    {
        painter->drawPixmap(lastTreefrontX,treeFrontOffset,treefront.width(),    treefront.height(),    treefront);
        lastTreefrontX+=treefront.width();
    }
    int lastGrassX=-grassMove;
    while(widget->width()>lastGrassX)
    {
        painter->drawPixmap(lastGrassX,grassOffset,grass.width(),    grass.height(),    grass);
        lastGrassX+=grass.width();
    }

    painter->drawPixmap(widget->width()*2/3/*66%*/-sun.width()/2,sunOffset,sun.width(),    sun.height(),    sun);
    //painter->drawPixmap(widget->width()-cloudMove,skyOffset+(endOfGrass-skyOffset)/4/*16%*/,cloud.width(),    cloud.height(),    cloud);

    int lastcloudX=-cloudMove;
    while(widget->width()>lastcloudX)
    {
        painter->drawPixmap(lastcloudX,skyOffset+(endOfGrass-skyOffset)/4/*16%*/,cloud.width(),    cloud.height(),    cloud);
        lastcloudX+=cloud.width();
    }

    switch(animationStep)
    {
        case 0:
        {
            if(floorMove<widget->width()/2)
            {
                const int frameTimeCount=150;
                const int tempF=floorMove%frameTimeCount;//0 to 149
                const int frameCount=2;
                const int targetFrame=tempF*frameCount/frameTimeCount;
                if(player1Frame!=targetFrame)
                {
                    player1Frame=targetFrame;
                    switch(targetFrame)
                    {
                        case 0:
                        player1=QPixmap(":/copyright/1.png");
                        if(player1.isNull())
                            abort();
                        break;
                        case 1:
                        player1=QPixmap(":/copyright/2.png");
                        if(player1.isNull())
                            abort();
                        break;
                        default:
                        break;
                    }

                    scalePix(player1,targetZoom);
                }
            }
            else
            {

                if(!pauseBeforeDisplayDialog.isActive())
                    pauseBeforeDisplayDialog.start();
                animationStep=1;
                grassTimer.stop();
                treefrontTimer.stop();
                treebackTimer.stop();
            }
            painter->drawPixmap(floorMove-player1.width(),grassOffset+grass.height()-player1.height(),player1.width(),    player1.height(),    player1);
            painter->drawPixmap(widget->width()-floorMove,grassOffset+grass.height()-player2.height(),player2.width(),    player2.height(),    player2);
        }
        break;
        case 1:
        {
            const int offsetBorder=40;
            if(!dialog1Text.empty())
            {
                painter->drawPixmap(widget->width()/2-dialog1.width(),grassOffset+grass.height()-player1.height()-dialog1.height(),dialog1.width(),    dialog1.height(),    dialog1);
                painter->setPen(QColor(25,11,0));
                //painter->drawText(widget->width()/2-dialog1.width()+offsetBorder,grassOffset+grass.height()-player1.height()-dialog1.height()+offsetBorder,QString::fromStdString(dialog1Text));

                painter->save();
                painter->translate(widget->width()/2-dialog1.width()+offsetBorder,grassOffset+grass.height()-player1.height()-dialog1.height()+offsetBorder);
                dialog1Qt.paint(painter,option,widget);
                dialog1Qt.setTextWidth(420);
                dialog1Qt.setHtml("<span style=\"color:#401c02\">"+QString::fromStdString(dialog1Text)+"</span>");
                painter->restore();
            }
            if(!dialog2Text.empty())
            {
                painter->drawPixmap(widget->width()/2,grassOffset+grass.height()-player1.height()-dialog2.height(),dialog2.width(),    dialog2.height(),    dialog2);
                painter->setPen(QColor(25,11,0));
                //painter->drawText(widget->width()/2+offsetBorder,grassOffset+grass.height()-player1.height()-dialog2.height()+offsetBorder,QString::fromStdString(dialog2Text));

                painter->save();
                painter->translate(widget->width()/2+offsetBorder,grassOffset+grass.height()-player1.height()-dialog2.height()+offsetBorder);
                dialog1Qt.paint(painter,option,widget);
                dialog1Qt.setTextWidth(420);
                dialog1Qt.setHtml("<span style=\"color:#401c02\">"+QString::fromStdString(dialog2Text)+"</span>");
                painter->restore();
            }
            painter->drawPixmap(widget->width()/2-player1.width(),grassOffset+grass.height()-player1.height(),player1.width(),    player1.height(),    player1);
            painter->drawPixmap(widget->width()/2,grassOffset+grass.height()-player2.height(),player2.width(),    player2.height(),    player2);
        }
        break;
        default:
        break;
    }
}
