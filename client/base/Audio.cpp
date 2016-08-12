#ifndef CATCHCHALLENGER_NOAUDIO
#include "Audio.h"
#include "PlatformMacro.h"
#include "../../general/base/GeneralVariable.h"
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>

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

void Audio::setVolume(const int &volume)
{
    qDebug() << "Audio volume set to: " << volume;
    int index=0;
    while(index<playerList.size())
    {
        libvlc_audio_set_volume(playerList.at(index),volume);
        libvlc_audio_set_mute(playerList.at(index),0);
        index++;
    }
    this->volume=volume;
}

void Audio::addPlayer(libvlc_media_player_t * const player)
{
    if(playerList.contains(player))
        return;
    playerList << player;
    libvlc_audio_set_volume(player,volume);
    libvlc_audio_set_mute(player,0);
}

void Audio::setPlayerVolume(libvlc_media_player_t * const player)
{
    libvlc_audio_set_volume(player,volume);
    libvlc_audio_set_mute(player,0);
}

void Audio::removePlayer(libvlc_media_player_t * const player)
{
    playerList.removeOne(player);
}

QStringList Audio::output_list()
{
    QStringList outputs;
    /*libvlc_audio_output_device_t * output=libvlc_audio_output_device_list_get(vlcInstance,NULL);
    do
    {
        outputs << output->psz_device;
        output=output->p_next;
    } while(output!=NULL);*/
    return outputs;
}

Audio::~Audio()
{
    /* Release libVLC instance on quit */
    if(vlcInstance)
        libvlc_release(vlcInstance);
}
#endif
