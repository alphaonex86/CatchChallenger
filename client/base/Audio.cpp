#include "Audio.h"
#include "PlatformMacro.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralVariable.h"
#include <QCoreApplication>
#include <QSettings>

Audio Audio::audio;

Audio::Audio()
{
    QSettings settings;
    if(!settings.contains(QStringLiteral("audio_init")) || settings.value(QStringLiteral("audio_init")).toInt()==2)
    {
        settings.setValue(QStringLiteral("audio_init"),1);
        settings.sync();
        /* Initialize libVLC */
        /*const char * const vlc_args[] = {
              "-vvv"
        };
        vlcInstance = libvlc_new(1,vlc_args);*/
        vlcInstance = libvlc_new(0,NULL);
        /* Complain in case of broken installation */
        if (vlcInstance == NULL)
            qDebug() << "Qt libVLC player, Could not init libVLC";
        const char * string=libvlc_errmsg();
        if(string!=NULL)
            qDebug() << string;
        settings.setValue(QStringLiteral("audio_init"),2);
    }
    else
        qDebug() << "Audio disabled due to previous crash";
}

Audio::~Audio()
{
    /* Release libVLC instance on quit */
    if(vlcInstance)
        libvlc_release(vlcInstance);
}
