#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.hpp"
#include "../qt/GameLoader.hpp"
#include <QCoreApplication>

AudioGL::AudioGL()
{
}

AudioGL::~AudioGL()
{
}

//if already playing ambiance then call stopCurrentAmbiance
std::string AudioGL::startAmbiance(const std::string &soundPath)
{
    if(QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).isEmpty())
            return "No audio device found!";

    const QString QtsoundPath=QString::fromStdString(soundPath);
    if(!QtsoundPath.startsWith(":/"))
        return Audio::startAmbiance(soundPath);
    stopCurrentAmbiance();

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(m_format))
        return std::to_string((int)m_format.byteOrder())+","+
                std::to_string((int)m_format.channelCount())+","+
                m_format.codec().toStdString()+","+
                std::to_string((int)m_format.isValid())+","+
                std::to_string((int)m_format.sampleRate())+","+
                std::to_string((int)m_format.sampleType())+","+
                " "+QAudioDeviceInfo::defaultOutputDevice().deviceName().toStdString();

    ambiance_player = new QAudioOutput(Audio::audio->format());
    // Create a new Media
    if(ambiance_player!=nullptr && GameLoader::gameLoader->musics.contains(QtsoundPath))
    {
        ambiance_data=*GameLoader::gameLoader->musics.value(QtsoundPath);
        ambiance_buffer=new QInfiniteBuffer(&ambiance_data);
        ambiance_buffer->open(QBuffer::ReadOnly);
        ambiance_buffer->seek(0);
        ambiance_player->start(ambiance_buffer);
        addPlayer(ambiance_player);
        return std::string();
    }
    if(ambiance_player!=nullptr)
        return "QAudioOutput nullptr";
    else
        return "!GameLoader::gameLoader->musics.contains(QtsoundPath)";
}

void AudioGL::stopCurrentAmbiance()
{
    Audio::stopCurrentAmbiance();
}
#endif
