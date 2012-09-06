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

QString Dialog::loadOtherMap(const QString &fileName)
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
    other_map[resolvedFileName].width                                 = map_loader.map_to_send.width;
    other_map[resolvedFileName].height                                = map_loader.map_to_send.height;
    other_map[resolvedFileName].parsed_layer.walkable                 = map_loader.map_to_send.parsed_layer.walkable;
    other_map[resolvedFileName].parsed_layer.water                    = map_loader.map_to_send.parsed_layer.water;
    other_map[resolvedFileName].map_file                              = resolvedFileName;
    other_map[resolvedFileName].border.bottom.map                     = NULL;
    other_map[resolvedFileName].border.top.map                        = NULL;
    other_map[resolvedFileName].border.right.map                      = NULL;
    other_map[resolvedFileName].border.left.map                       = NULL;

    //load the string
    other_map[resolvedFileName].border_semi                = map_loader.map_to_send.border;

    QString mapIndex;

    //if have border
    if(!other_map[resolvedFileName].border_semi.bottom.fileName.isEmpty())
    {
        other_map[resolvedFileName].border_semi.bottom.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].border_semi.bottom.fileName).absoluteFilePath();
        if(!other_map.contains(other_map[resolvedFileName].border_semi.bottom.fileName))
        {
            map_log[resolvedFileName]<<Pokecraft::DebugClass::getLog();
            mapIndex=loadOtherMap(other_map[resolvedFileName].border_semi.bottom.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(resolvedFileName==other_map[mapIndex].border_semi.top.fileName && other_map[resolvedFileName].border_semi.bottom.fileName==mapIndex)
                {
                    other_map[resolvedFileName].border.bottom.map=&other_map[mapIndex];
                    int offset=other_map[resolvedFileName].border.bottom.x_offset-other_map[mapIndex].border.top.x_offset;
                    other_map[resolvedFileName].border.bottom.x_offset=offset;
                    other_map[mapIndex].border.top.x_offset=-offset;
                }
                else
                    Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border bottom, because the map at the bottom have not this map as border").arg(resolvedFileName));
            }
            else
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].border_semi.bottom.fileName));
        }
    }

    //if have border
    if(!other_map[resolvedFileName].border_semi.top.fileName.isEmpty())
    {
        other_map[resolvedFileName].border_semi.top.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].border_semi.top.fileName).absoluteFilePath();
        if(!other_map.contains(other_map[resolvedFileName].border_semi.top.fileName))
        {
            map_log[resolvedFileName]<<Pokecraft::DebugClass::getLog();
            mapIndex=loadOtherMap(other_map[resolvedFileName].border_semi.top.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(resolvedFileName==other_map[mapIndex].border_semi.bottom.fileName && other_map[resolvedFileName].border_semi.top.fileName==mapIndex)
                {
                    other_map[resolvedFileName].border.top.map=&other_map[mapIndex];
                    int offset=other_map[resolvedFileName].border.top.x_offset-other_map[mapIndex].border.bottom.x_offset;
                    other_map[resolvedFileName].border.top.x_offset=offset;
                    other_map[mapIndex].border.bottom.x_offset=-offset;
                }
                else
                    Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border top, because the map at the top have not this map as border").arg(resolvedFileName));
            }
            else
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].border_semi.top.fileName));
        }
    }

    //if have border
    if(!other_map[resolvedFileName].border_semi.left.fileName.isEmpty())
    {
        other_map[resolvedFileName].border_semi.left.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].border_semi.left.fileName).absoluteFilePath();
        if(!other_map.contains(other_map[resolvedFileName].border_semi.left.fileName))
        {
            map_log[resolvedFileName]<<Pokecraft::DebugClass::getLog();
            mapIndex=loadOtherMap(other_map[resolvedFileName].border_semi.left.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(resolvedFileName==other_map[mapIndex].border_semi.right.fileName && other_map[resolvedFileName].border_semi.left.fileName==mapIndex)
                {
                    other_map[resolvedFileName].border.left.map=&other_map[mapIndex];
                    int offset=other_map[resolvedFileName].border.left.y_offset-other_map[mapIndex].border.right.y_offset;
                    other_map[resolvedFileName].border.left.y_offset=offset;
                    other_map[mapIndex].border.right.y_offset=-offset;
                }
                else
                    Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border left, because the map at the left have not this map as border").arg(resolvedFileName));
            }
            else
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].border_semi.left.fileName));
        }
    }

    //if have border
    if(!other_map[resolvedFileName].border_semi.right.fileName.isEmpty())
    {
        other_map[resolvedFileName].border_semi.right.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+other_map[resolvedFileName].border_semi.right.fileName).absoluteFilePath();
        if(!other_map.contains(other_map[resolvedFileName].border_semi.right.fileName))
        {
            map_log[resolvedFileName]<<Pokecraft::DebugClass::getLog();
            mapIndex=loadOtherMap(other_map[resolvedFileName].border_semi.right.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(resolvedFileName==other_map[mapIndex].border_semi.left.fileName && other_map[resolvedFileName].border_semi.right.fileName==mapIndex)
                {
                    other_map[resolvedFileName].border.right.map=&other_map[mapIndex];
                    int offset=other_map[resolvedFileName].border.right.y_offset-other_map[mapIndex].border.left.y_offset;
                    other_map[resolvedFileName].border.right.y_offset=offset;
                    other_map[mapIndex].border.left.y_offset=-offset;
                }
                else
                    Pokecraft::DebugClass::debugConsole(QString("the map: %1 is not linked with the border right, because the map at the right have not this map as border").arg(resolvedFileName));
            }
            else
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 can't load the border: %2").arg(resolvedFileName).arg(other_map[resolvedFileName].border_semi.right.fileName));
        }
    }

    //check the teleporter
    int index=0;
    while(index<other_map[resolvedFileName].teleport_semi.size())
    {
        mapIndex=loadOtherMap(other_map[resolvedFileName].teleport_semi[index].map);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if both border match
            if(other_map[resolvedFileName].teleport_semi[index].destination_x<other_map[mapIndex].width && other_map[resolvedFileName].teleport_semi[index].destination_y<other_map[mapIndex].height)
            {
            }
            else
                Pokecraft::DebugClass::debugConsole(QString("the map: %1 have teleporter witch point out of the map %2").arg(resolvedFileName).arg(mapIndex));
        }
        else
            Pokecraft::DebugClass::debugConsole(QString("the map: %1 have teleporter with wrong destination %2").arg(resolvedFileName).arg(other_map[resolvedFileName].teleport_semi[index].map));
        index++;
    }

    //load the map
    Tiled::Map *tiledMap = reader.readMap(resolvedFileName);
    if(!tiledMap)
        Pokecraft::DebugClass::debugConsole(QString("the map: %1 have bug into the render with tiled: %2").arg(resolvedFileName).arg(reader.errorString()));
    else
        delete tiledMap;

    map_log[resolvedFileName]<<Pokecraft::DebugClass::getLog();
    return resolvedFileName;
}

void Dialog::scanMaps(const QString &folderName)
{
    Pokecraft::DebugClass::redirection=true;

    QTime startTime;
    startTime.restart();

    scanFolder(QDir(folderName));

    qDebug() << startTime.elapsed();

    QString html="<ul>";
    QHash<QString,QStringList>::const_iterator i = map_log.constBegin();
    while (i != map_log.constEnd()) {
        if(i.value().size()>0)
        {
            html+="<li>"+i.key();
            html+="<ul><li>"+i.value().join("</li><li>")+"</li></ul>";
            html+="</li>";
        }
        ++i;
    }
    html+="</ul>";
    ui->textEdit->setHtml(html);
}

void Dialog::scanFolder(const QDir &dir)
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
                loadOtherMap(dir.absolutePath()+'/'+fileInfo.fileName());
        }
        else
        {
            //return the fonction for scan the new folder
            scanFolder(dir.absolutePath()+'/'+fileInfo.fileName()+'/');
        }
    }
}
