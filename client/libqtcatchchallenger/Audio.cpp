#ifndef CATCHCHALLENGER_NOAUDIO
#include "Audio.hpp"
#include "PlatformMacro.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "Settings.hpp"
#include <QCoreApplication>
#include <iostream>

Audio *Audio::audio=nullptr;

Audio::Audio()
{
    //init audio here
    m_format.setSampleRate(48000);
    m_format.setChannelCount(2);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(m_format)) {
        std::cerr << "raw audio format not supported by backend, cannot play audio." << std::endl;
        return;
    }

    volume=100;
    ambiance_player=nullptr;
    ambiance_buffer=nullptr;
}

QAudioFormat Audio::format() const
{
    return m_format;
}

void Audio::setVolume(const int &volume)
{
    std::cout << "Audio volume set to: " << volume << std::endl;
    /*unsigned int index=0;
    while(index<playerList.size())
    {
        playerList.at(index)->setVolume((qreal)volume/100);
        index++;
    }*/
    this->volume=volume;
}

void Audio::addPlayer(QAudioOutput * const player)
{
    if(vectorcontainsAtLeastOne(playerList,player))
        return;
    playerList.push_back(player);
    player->setVolume((qreal)volume/100);
}

void Audio::removePlayer(QAudioOutput * const player)
{
    vectorremoveOne(playerList,player);
}

void Audio::setPlayerVolume(QAudioOutput * const player)
{
    player->setVolume((qreal)volume/100);
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
    stopCurrentAmbiance();
}

bool Audio::decodeOpus(const std::string &filePath,QByteArray &data)
{
    QBuffer buffer(&data);
    buffer.open(QBuffer::ReadWrite);

    int           ret;
    OggOpusFile  *of=op_open_file(filePath.c_str(),&ret);
    if(of==NULL) {
        fprintf(stderr,"Audio Failed to open file '%s': %i\n",filePath.c_str(),ret);
        return false;
    }
    ogg_int64_t pcm_offset;
    ogg_int64_t nsamples;
    nsamples=0;
    pcm_offset=op_pcm_tell(of);
    if(pcm_offset!=0)
        fprintf(stderr,"Non-zero starting PCM offset: %li\n",(long)pcm_offset);
    for(;;) {
        ogg_int64_t   next_pcm_offset;
        opus_int16    pcm[120*48*2];
        unsigned char out[120*48*2*2];
        int           si;
        ret=op_read_stereo(of,pcm,sizeof(pcm)/sizeof(*pcm));
        if(ret==OP_HOLE) {
            fprintf(stderr,"\nHole detected! Corrupt file segment?\n");
            continue;
        }
        else if(ret<0) {
            fprintf(stderr,"\nError decoding '%s': %i\n","file.opus",ret);
            ret=EXIT_FAILURE;
            break;
        }
        next_pcm_offset=op_pcm_tell(of);
        pcm_offset=next_pcm_offset;
        if(ret<=0) {
            ret=EXIT_SUCCESS;
            break;
        }
        for(si=0;si<2*ret;si++) { /// Ensure the data is little-endian before writing it out.
            out[2*si+0]=(unsigned char)(pcm[si]&0xFF);
            out[2*si+1]=(unsigned char)(pcm[si]>>8&0xFF);
        }
        buffer.write(reinterpret_cast<char *>(out),sizeof(*out)*4*ret);
        nsamples+=ret;
    }
    op_free(of);

    buffer.seek(0);
    return ret==EXIT_SUCCESS;
}

//if already playing ambiance then call stopCurrentAmbiance
std::string Audio::startAmbiance(const std::string &soundPath)
{
    stopCurrentAmbiance();

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(m_format))
        return "raw audio format not supported by backend, cannot play audio.";

    ambiance_player = new QAudioOutput(Audio::audio->format());
    // Create a new Media
    if(ambiance_player!=nullptr)
    {
        //decode file
        if(Audio::decodeOpus(soundPath,ambiance_data))
        {
            ambiance_buffer=new QInfiniteBuffer(&ambiance_data);
            ambiance_buffer->open(QBuffer::ReadOnly);
            ambiance_buffer->seek(0);
            ambiance_player->start(ambiance_buffer);

            addPlayer(ambiance_player);
            return std::string();
        }
        else
        {
            delete ambiance_player;
            ambiance_player=nullptr;
            ambiance_data.clear();
            return "Audio::decodeOpus failed";
        }
    }
    return "ambiance_player==nullptr";
}

void Audio::stopCurrentAmbiance()
{
    if(ambiance_player!=nullptr)
    {
        removePlayer(ambiance_player);
        ambiance_player->stop();
        delete ambiance_player;
        ambiance_player=nullptr;
        ambiance_data.clear();
    }
}
#endif
