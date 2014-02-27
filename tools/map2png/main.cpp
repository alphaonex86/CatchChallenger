/*
 * main.cpp
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of the TMX Viewer example.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "map2png.h"

#include <QApplication>
#include <QFileDialog>
#include <QStringList>
#include <QString>
#include <QFileInfoList>
#include <QFileInfo>
#include <QDir>
#include <QTime>

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map2png"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    const QStringList &arguments=QCoreApplication::arguments();
    QString fileToOpen,destination;
    QFileInfo dir;
    if(arguments.size()==2)
        dir=QFileInfo(arguments.last());
    if(arguments.size()==2 && dir.isDir())
    {
        QTime time;
        time.restart();
        QStringList files=Map2Png::listFolder(dir.absoluteFilePath()+Map2Png::text_slash);
        int index=0;
        while(index<files.size())
        {
            if(files.at(index).endsWith(Map2Png::text_dottmx))
            {
                fileToOpen=files.at(index);
                destination=files.at(index);
                destination.replace(Map2Png::text_dottmx,Map2Png::text_dotpng);
                Map2Png w;
                w.viewMap(false,dir.absoluteFilePath()+Map2Png::text_slash+fileToOpen,destination);
            }
            index++;
        }
        qDebug() << QString("Done into: %1s").arg(time.elapsed()/1000);
        return 0;
    }
    else
    {
        {
            int index=0;
            while(index<arguments.size())
            {
                if(arguments.at(index).endsWith(Map2Png::text_dottmx))
                {
                    fileToOpen=arguments.at(index);
                    break;
                }
                index++;
            }
            {
                int index=0;
                while(index<arguments.size())
                {
                    if(arguments.at(index).endsWith(Map2Png::text_dotpng))
                    {
                        destination=arguments.at(index);
                        break;
                    }
                    index++;
                }
            }
        }
        Map2Png w;
        QString previousFolder;
        if (fileToOpen.isEmpty())
        {
            QString source = QFileDialog::getOpenFileName(NULL,QStringLiteral("Select map"));
            if(source.isEmpty() || source.isNull())
                return 0;
            fileToOpen=source;
        }
        if(destination.isEmpty())
        {
            destination = QFileDialog::getSaveFileName(NULL,QStringLiteral("Save the render"),QString(),QStringLiteral("Png Images (*.png)"));
            if(destination.isEmpty())
            {
            }
            else
            {
                if(!destination.endsWith(Map2Png::text_dotpng))
                    destination+=Map2Png::text_dotpng;
            }
        }
        {
            bool found=true;
            QFileInfo dir(QFileInfo(fileToOpen).absolutePath());
            while(!QFileInfo(dir.absoluteFilePath()+QStringLiteral("/informations.xml")).exists())
            {
                previousFolder=dir.absoluteFilePath();
                dir=QFileInfo(dir.absolutePath());
                if(previousFolder==dir.absoluteFilePath())
                {
                    found=false;
                    break;
                }
            }
            if(found)
                w.baseDatapack=dir.absoluteFilePath();
        }

        if(arguments.size()==1)
        {
            w.viewMap(true,fileToOpen,destination);
            w.show();
            w.setWindowIcon(QIcon(QStringLiteral(":/icon.png")));
            return a.exec();
        }
        else
        {
            w.viewMap(false,fileToOpen,destination);
            return 0;
        }
    }
}
