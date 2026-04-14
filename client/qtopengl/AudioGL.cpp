#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.hpp"
#include "GameLoader.hpp"
#include <QCoreApplication>
#include <QMediaDevices>
#include <QAudioDevice>

AudioGL::AudioGL()
{
}

AudioGL::~AudioGL()
{
}

//if already playing ambiance then call stopCurrentAmbiance
std::string AudioGL::startAmbiance(const std::string &soundPath)
{
    const QString QtsoundPath=QString::fromStdString(soundPath);
    if(!QtsoundPath.startsWith(":/"))
        return Audio::startAmbiance(soundPath);
    stopCurrentAmbiance();

    QAudioDevice info=QMediaDevices::defaultAudioOutput();
    if (!info.isFormatSupported(m_format))
        return std::to_string((int)m_format.channelCount())+","+
                std::to_string((int)m_format.isValid())+","+
                std::to_string((int)m_format.sampleRate())+","+
                " "+info.description().toStdString();

    if(GameLoader::gameLoader!=nullptr && GameLoader::gameLoader->musics.contains(QtsoundPath))
    {
        ambiance_player = new QAudioSink(info,Audio::audio->format());
        if(ambiance_player!=nullptr)
        {
            ambiance_data=*GameLoader::gameLoader->musics.value(QtsoundPath);
            ambiance_buffer=new QInfiniteBuffer(&ambiance_data);
            ambiance_buffer->open(QBuffer::ReadOnly);
            ambiance_buffer->seek(0);
            ambiance_player->start(ambiance_buffer);
            addPlayer(ambiance_player);
            return std::string();
        }
        return "QAudioSink nullptr";
    }
    else
        return "!GameLoader::gameLoader->musics.contains("+QtsoundPath.toStdString()+")";
}

void AudioGL::stopCurrentAmbiance()
{
    Audio::stopCurrentAmbiance();
}
#endif
