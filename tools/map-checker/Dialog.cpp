#include "Dialog.h"
#include "ui_Dialog.h"

#include <QFileInfo>

#include "../../general/base/DebugClass.h"
#include "../../general/base/Map_loader.h"
#include "map.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

/** \todo check layer: Collisions, Walkable */
QString Dialog::loadMap(const QString &fileName)
{
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(other_map.contains(resolvedFileName))
        return resolvedFileName;

    Pokecraft::Map_loader map_loader;
    if(!map_loader.tryLoadMap(resolvedFileName))
    {
        Pokecraft::DebugClass::debugConsole(QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(map_loader.errorString()));
        return QString();
    }

    //copy the variables
    other_map[resolvedFileName].map.width                                 = map_loader.map_to_send.width;
    other_map[resolvedFileName].map.height                                = map_loader.map_to_send.height;
    delete map_loader.map_to_send.parsed_layer.walkable;
    delete map_loader.map_to_send.parsed_layer.water;
//    other_map[resolvedFileName].map.parsed_layer.walkable                 = map_loader.map_to_send.parsed_layer.walkable;
//    other_map[resolvedFileName].map.parsed_layer.water                    = map_loader.map_to_send.parsed_layer.water;
    other_map[resolvedFileName].map.map_file                              = resolvedFileName;
    other_map[resolvedFileName].position_set                              = false;

    //load the string
    other_map[resolvedFileName].map.border_semi                = map_loader.map_to_send.border;

    QString mapIndex;

    //if have border
    if(!other_map[resolvedFileName].map.border_semi.bottom.fileName.isEmpty())
    {
        other_map[resolvedFileName].map.border_semi.bottom.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].map.border_semi.bottom.fileName).absoluteFilePath();
        other_map[resolvedFileName].log<<Pokecraft::DebugClass::getLog();
        mapIndex=loadMap(other_map[resolvedFileName].map.border_semi.bottom.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(resolvedFileName==other_map[mapIndex].map.border_semi.top.fileName && other_map[resolvedFileName].map.border_semi.bottom.fileName==mapIndex)
            {
            }
            else
            {
                other_map[resolvedFileName].map.border_semi.bottom.fileName.clear();
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border bottom, because the map at the bottom have not this map as border").arg(resolvedFileName));
            }
        }
        else
        {
            other_map[resolvedFileName].map.border_semi.bottom.fileName.clear();
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.bottom.fileName));
        }
    }

    //if have border
    if(!other_map[resolvedFileName].map.border_semi.top.fileName.isEmpty())
    {
        other_map[resolvedFileName].map.border_semi.top.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].map.border_semi.top.fileName).absoluteFilePath();
        other_map[resolvedFileName].log<<Pokecraft::DebugClass::getLog();
        mapIndex=loadMap(other_map[resolvedFileName].map.border_semi.top.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(resolvedFileName==other_map[mapIndex].map.border_semi.bottom.fileName && other_map[resolvedFileName].map.border_semi.top.fileName==mapIndex)
            {
            }
            else
            {
                other_map[resolvedFileName].map.border_semi.top.fileName.clear();
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border top, because the map at the top have not this map as border").arg(resolvedFileName));
            }
        }
        else
        {
            other_map[resolvedFileName].map.border_semi.top.fileName.clear();
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.top.fileName));
        }
    }

    //if have border
    if(!other_map[resolvedFileName].map.border_semi.left.fileName.isEmpty())
    {
        other_map[resolvedFileName].map.border_semi.left.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].map.border_semi.left.fileName).absoluteFilePath();
        other_map[resolvedFileName].log<<Pokecraft::DebugClass::getLog();
        mapIndex=loadMap(other_map[resolvedFileName].map.border_semi.left.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(resolvedFileName==other_map[mapIndex].map.border_semi.right.fileName && other_map[resolvedFileName].map.border_semi.left.fileName==mapIndex)
            {
            }
            else
            {
                other_map[resolvedFileName].map.border_semi.left.fileName.clear();
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border left, because the map at the left have not this map as border").arg(resolvedFileName));
            }
        }
        else
        {
            other_map[resolvedFileName].map.border_semi.left.fileName.clear();
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.left.fileName));
        }
    }

    //if have border
    if(!other_map[resolvedFileName].map.border_semi.right.fileName.isEmpty())
    {
        other_map[resolvedFileName].map.border_semi.right.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].map.border_semi.right.fileName).absoluteFilePath();
        other_map[resolvedFileName].log<<Pokecraft::DebugClass::getLog();
        mapIndex=loadMap(other_map[resolvedFileName].map.border_semi.right.fileName);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(resolvedFileName==other_map[mapIndex].map.border_semi.left.fileName && other_map[resolvedFileName].map.border_semi.right.fileName==mapIndex)
            {
            }
            else
            {
                other_map[resolvedFileName].map.border_semi.right.fileName.clear();
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border right the map at the right have not this map as border").arg(resolvedFileName));
            }
        }
        else
        {
            other_map[resolvedFileName].map.border_semi.right.fileName.clear();
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.right.fileName));
        }
    }

    //check the teleporter
    int index=0;
    while(index<other_map[resolvedFileName].map.teleport_semi.size())
    {
        mapIndex=loadMap(other_map[resolvedFileName].map.teleport_semi[index].map);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(other_map[resolvedFileName].map.teleport_semi[index].destination_x<other_map[mapIndex].map.width && other_map[resolvedFileName].map.teleport_semi[index].destination_y<other_map[mapIndex].map.height)
            {
            }
            else
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 have teleporter witch point out of the map %2").arg(resolvedFileName).arg(mapIndex));
        }
        else
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 have teleporter with wrong destination %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.teleport_semi[index].map));
        index++;
    }

    //load the map
    Tiled::Map *tiledMap = reader.readMap(resolvedFileName);
    if(!tiledMap)
        Pokecraft::DebugClass::debugConsole(QString("the map: %1 have bug into the render with tiled: %2").arg(resolvedFileName).arg(reader.errorString()));
    else
        delete tiledMap;

    other_map[resolvedFileName].log<<Pokecraft::DebugClass::getLog();
    return resolvedFileName;
}

void Dialog::loadPosition(const QString &fileName)
{
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(!other_map.contains(resolvedFileName))
        return;

    //load the first position
    if(!other_map[resolvedFileName].position_set)
    {
        other_map[resolvedFileName].position_set=true;
        other_map[resolvedFileName].x=0;
        other_map[resolvedFileName].y=0;
    }

    //if have border
    QString bottom=other_map[resolvedFileName].map.border_semi.bottom.fileName;
    if(!bottom.isEmpty())
    {
        if(!other_map[bottom].position_set)
        {
            other_map[bottom].position_set=true;
            other_map[bottom].x=other_map[resolvedFileName].x+(other_map[resolvedFileName].map.border_semi.bottom.x_offset-other_map[bottom].map.border_semi.top.x_offset);
            other_map[bottom].y=other_map[resolvedFileName].y+other_map[resolvedFileName].map.height;
        }
        else
        {
            if(
                    other_map[bottom].x!=other_map[resolvedFileName].x+(other_map[resolvedFileName].map.border_semi.bottom.x_offset-other_map[bottom].map.border_semi.top.x_offset)
                    ||
                    other_map[bottom].y!=other_map[resolvedFileName].y+other_map[resolvedFileName].map.height
                    )
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 have the bottom border at the wrong position: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.bottom.fileName));
        }
    }

    //if have border
    QString top=other_map[resolvedFileName].map.border_semi.top.fileName;
    if(!top.isEmpty())
    {
        if(!other_map[top].position_set)
        {
            other_map[top].position_set=true;
            other_map[top].x=other_map[resolvedFileName].x+(other_map[resolvedFileName].map.border_semi.top.x_offset-other_map[top].map.border_semi.bottom.x_offset);
            other_map[top].y=other_map[resolvedFileName].y+other_map[resolvedFileName].map.height;
        }
        else
        {
            if(
                    other_map[top].x!=other_map[resolvedFileName].x+(other_map[resolvedFileName].map.border_semi.top.x_offset-other_map[top].map.border_semi.bottom.x_offset)
                    ||
                    other_map[top].y!=other_map[resolvedFileName].y-other_map[top].map.height
                    )
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 have the top border at the wrong position: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.top.fileName));
        }
    }

    //if have border
    QString right=other_map[resolvedFileName].map.border_semi.right.fileName;
    if(!right.isEmpty())
    {
        if(!other_map[right].position_set)
        {
            other_map[right].position_set=true;
            other_map[right].x=other_map[resolvedFileName].x+other_map[resolvedFileName].map.width;
            other_map[right].y=other_map[resolvedFileName].y+(other_map[resolvedFileName].map.border_semi.right.y_offset-other_map[right].map.border_semi.left.y_offset);
        }
        else
        {
            if(
                    other_map[right].x!=other_map[resolvedFileName].x+other_map[resolvedFileName].map.width
                    ||
                    other_map[right].y!=other_map[resolvedFileName].y+(other_map[resolvedFileName].map.border_semi.right.y_offset-other_map[right].map.border_semi.left.y_offset)
                    )
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 have the right border at the wrong position: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.right.fileName));
        }
    }

    //if have border
    QString left=other_map[resolvedFileName].map.border_semi.left.fileName;
    if(!left.isEmpty())
    {
        if(!other_map[left].position_set)
        {
            other_map[left].position_set=true;
            other_map[left].x=other_map[resolvedFileName].x+other_map[resolvedFileName].map.width;
            other_map[left].y=other_map[resolvedFileName].y+(other_map[resolvedFileName].map.border_semi.left.y_offset-other_map[left].map.border_semi.right.y_offset);
        }
        else
        {
            if(
                    other_map[left].x!=other_map[resolvedFileName].x+other_map[resolvedFileName].map.width
                    ||
                    other_map[left].y!=other_map[resolvedFileName].y+(other_map[resolvedFileName].map.border_semi.left.y_offset-other_map[left].map.border_semi.right.y_offset)
                    )
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 have the left border at the wrong position: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].map.border_semi.left.fileName));
        }
    }
}

void Dialog::scanMaps(const QString &folderName)
{
    Pokecraft::DebugClass::redirection=true;

    QTime startTime;
    startTime.restart();

    scanFolderMap(QDir(folderName));
    scanFolderPosition(QDir(folderName));

    qDebug() << startTime.elapsed();

    QString html="<ul>";
    QHash<QString,Map_full>::const_iterator i = other_map.constBegin();
    while (i != other_map.constEnd()) {
        if(i.value().log.size()>0)
        {
            html+="<li>"+i.key();
            html+="<ul><li>"+i.value().log.join("</li><li>")+"</li></ul>";
            html+="</li>";
        }
        ++i;
    }
    html+="</ul>";
    ui->textEdit->setHtml(html);
}

void Dialog::scanFolderMap(const QDir &dir)
{
    if(!dir.exists())
        return;

    QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            if(fileInfo.fileName().endsWith(".tmx"))
                loadMap(dir.absolutePath()+'/'+fileInfo.fileName());
        }
        else
        {
            //return the fonction for scan the new folder
            scanFolderMap(dir.absolutePath()+'/'+fileInfo.fileName()+'/');
        }
    }
}

void Dialog::scanFolderPosition(const QDir &dir)
{
    if(!dir.exists())
        return;

    QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            if(fileInfo.fileName().endsWith(".tmx"))
                loadPosition(dir.absolutePath()+'/'+fileInfo.fileName());
        }
        else
        {
            //return the fonction for scan the new folder
            scanFolderPosition(dir.absolutePath()+'/'+fileInfo.fileName()+'/');
        }
    }
}
