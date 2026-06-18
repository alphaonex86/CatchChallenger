#include "SkinResolver.hpp"

#include <QDir>
#include <QFileInfoList>
#include <cstdlib>
#include <iostream>

SkinResolver::SkinResolver(const std::string &skinBotDir, int perChannelTolerance) :
    skinBotDir_(skinBotDir),
    tolerance_(perChannelTolerance),
    nextNewId_(1),
    added_(0),
    reused_(0)
{
}

QImage SkinResolver::cropToContent(const QImage &src)
{
    QImage img=src.convertToFormat(QImage::Format_ARGB32);
    int minX=img.width(),minY=img.height(),maxX=-1,maxY=-1;
    int y=0;
    while(y<img.height())
    {
        int x=0;
        while(x<img.width())
        {
            if(qAlpha(img.pixel(x,y))>0)
            {
                if(x<minX) minX=x;
                if(x>maxX) maxX=x;
                if(y<minY) minY=y;
                if(y>maxY) maxY=y;
            }
            x++;
        }
        y++;
    }
    if(maxX<minX || maxY<minY)
        return QImage(); // fully transparent
    return img.copy(minX,minY,maxX-minX+1,maxY-minY+1);
}

bool SkinResolver::sameImage(const QImage &a, const QImage &b) const
{
    if(a.width()!=b.width() || a.height()!=b.height())
        return false;
    int y=0;
    while(y<a.height())
    {
        int x=0;
        while(x<a.width())
        {
            QRgb pa=a.pixel(x,y);
            QRgb pb=b.pixel(x,y);
            if(std::abs(qRed(pa)-qRed(pb))>tolerance_) return false;
            if(std::abs(qGreen(pa)-qGreen(pb))>tolerance_) return false;
            if(std::abs(qBlue(pa)-qBlue(pb))>tolerance_) return false;
            if(std::abs(qAlpha(pa)-qAlpha(pb))>tolerance_) return false;
            x++;
        }
        y++;
    }
    return true;
}

void SkinResolver::loadExisting()
{
    QDir dir(QString::fromStdString(skinBotDir_));
    QFileInfoList list=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
    int i=0;
    int maxId=0;
    while(i<list.size())
    {
        QString name=list.at(i).fileName();
        bool ok=false;
        int idValue=name.toInt(&ok);
        if(ok && idValue>maxId)
            maxId=idValue;
        QString trainer=list.at(i).absoluteFilePath()+"/trainer.png";
        if(QFile::exists(trainer))
        {
            QImage img(trainer);
            if(!img.isNull())
            {
                Entry e;
                e.name=name.toStdString();
                e.cropped=cropToContent(img);
                if(!e.cropped.isNull())
                    entries_.push_back(e);
            }
        }
        i++;
    }
    nextNewId_=maxId+1;
    std::cout << "SkinResolver: " << entries_.size() << " existing skins with trainer.png loaded; next new id "
              << nextNewId_ << std::endl;
}

std::string SkinResolver::resolve(const QImage &candidate)
{
    QImage cropped=cropToContent(candidate);
    if(cropped.isNull())
        return std::string();
    size_t i=0;
    while(i<entries_.size())
    {
        if(sameImage(cropped,entries_[i].cropped))
        {
            reused_++;
            return entries_[i].name;
        }
        i++;
    }
    // No match: register a new skin.
    std::string name=std::to_string(nextNewId_);
    nextNewId_++;
    std::string folder=skinBotDir_+"/"+name;
    QDir().mkpath(QString::fromStdString(folder));
    const std::string skinPath=folder+"/trainer.png";
    candidate.save(QString::fromStdString(skinPath),"PNG");
    addedPaths_.push_back(skinPath); // for the PNG optimiser (each new skin once)
    Entry e;
    e.name=name;
    e.cropped=cropped;
    entries_.push_back(e);
    added_++;
    return name;
}

size_t SkinResolver::addedCount() const
{
    return added_;
}

size_t SkinResolver::reuseCount() const
{
    return reused_;
}
