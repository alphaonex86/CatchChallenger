#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.h"
#include "../qt/GameLoader.h"

AudioGL AudioGL::audioGL;

AudioGL::AudioGL()
{
}

AudioGL::~AudioGL()
{
}

//if already playing ambiance then call stopCurrentAmbiance
bool AudioGL::startAmbiance(const std::string &soundPath)
{
    const QString QtsoundPath=QString::fromStdString(soundPath);
    if(!QtsoundPath.startsWith(":/"))
        return Audio::startAmbiance(soundPath);
    stopCurrentAmbiance();
    ambiance_player = new QAudioOutput(Audio::audio.format());
    // Create a new Media
    if(ambiance_player!=nullptr && GameLoader::gameLoader->musics.contains(QtsoundPath))
    {
        ambiance_data=GameLoader::gameLoader->musics.value(QtsoundPath);
        ambiance_buffer=new QInfiniteBuffer(&ambiance_data);
        ambiance_buffer->open(QBuffer::ReadOnly);
        ambiance_buffer->seek(0);
        ambiance_player->start(ambiance_buffer);
        addPlayer(ambiance_player);
        return true;
    }
    return false;
}
#endif
