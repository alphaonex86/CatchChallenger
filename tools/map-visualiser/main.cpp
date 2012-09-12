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

#include "map-visualiser.h"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>

namespace {

struct CommandLineOptions {
    CommandLineOptions()
    {}

    QString fileToOpen;
};

} // anonymous namespace

static void parseCommandLineArguments(CommandLineOptions &options)
{
    const QStringList arguments = QCoreApplication::arguments();

    for (int i = 1; i < arguments.size(); ++i) {
        const QString &arg = arguments.at(i);
        if (arg.at(0) == QLatin1Char('-')) {
            qWarning() << "Unknown option" << arg;
        } else {
            options.fileToOpen = arg;
        }
    }
}

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QLatin1String("raster"));
#endif

    QApplication a(argc, argv);

    a.setOrganizationDomain(QLatin1String("pokecraft"));
    a.setApplicationName(QLatin1String("Map visualiser"));
    a.setApplicationVersion(QLatin1String("1.0"));

    CommandLineOptions options;
    parseCommandLineArguments(options);

    if (options.fileToOpen.isEmpty())
    {
        QString source = QFileDialog::getOpenFileName(NULL,"Select map");
        if(source.isEmpty() || source.isNull() || source=="")
            return 0;
        options.fileToOpen=source;
    }

    MapVisualiser w;
    w.viewMap(options.fileToOpen);
    w.show();

    return a.exec();
}
