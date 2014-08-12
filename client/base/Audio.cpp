#include "Audio.h"
#include "PlatformMacro.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralVariable.h"
#include <QCoreApplication>

Audio Audio::audio;

Audio::Audio()
{
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
}

Audio::~Audio()
{
    /* Release libVLC instance on quit */
    if(vlcInstance)
        libvlc_release(vlcInstance);
}
