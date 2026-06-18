#include "Audio.hpp"
#include "PlatformMacro.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/mp2k/Mp2kPlayer.hpp"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QFile>
#include <vector>
#include <cmath>
#include <iostream>

Audio *Audio::audio=nullptr;

Audio::Audio() :
    volume(0),
    ambiance_player(nullptr),
    ambiance_buffer(nullptr)
{
    // Init pointers + volume in the ctor init list (CLAUDE.md "always
    // init in constructor initializer list in .cpp", no in-class init):
    // the body below has an early-return for the
    // !info.isFormatSupported() path, so anything assigned only there
    // (the prior ambiance_player=nullptr at the bottom) would be left
    // as garbage when the early return fires — exactly the bug seen on
    // x86-lxc i386 (offscreen Qt platform, no audio backend) where
    // ~Audio()::stopCurrentAmbiance() then dereferenced a wild pointer
    // and SIGSEGV'd inside QAudioSink::stop().  Initing in the list
    // guarantees the members are valid even if the ctor body bails.
    //init audio here
    m_format.setSampleRate(48000);
    m_format.setChannelCount(2);
    m_format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice info=QMediaDevices::defaultAudioOutput();
    if (!info.isFormatSupported(m_format)) {
        std::cerr << "raw audio format not supported by backend, cannot play audio." << std::endl;
        return;
    }

    const QString platform=QGuiApplication::platformName();
    if(platform==QLatin1String("offscreen") || platform==QLatin1String("minimal"))
        volume=0;
    else
        volume=100;
}

QAudioFormat Audio::format() const
{
    return m_format;
}

void Audio::setVolume(const int &volume)
{
    const QString platform=QGuiApplication::platformName();
    if(platform==QLatin1String("offscreen") || platform==QLatin1String("minimal"))
    {
        this->volume=0;
        return;
    }
    std::cout << "Audio volume set to: " << volume << std::endl;
    unsigned int index=0;
    while(index<playerList.size())
    {
        playerList.at(index)->setVolume((qreal)volume/100);
        index++;
    }
    this->volume=volume;
}

void Audio::addPlayer(QAudioSink * const player)
{
    if(vectorcontainsAtLeastOne(playerList,player))
        return;
    playerList.push_back(player);
    player->setVolume((qreal)volume/100);
}

void Audio::removePlayer(QAudioSink * const player)
{
    vectorremoveOne(playerList,player);
}

void Audio::setPlayerVolume(QAudioSink * const player)
{
    const QString platform=QGuiApplication::platformName();
    if(platform==QLatin1String("offscreen") || platform==QLatin1String("minimal"))
    {
        player->setVolume(0.0);
        return;
    }
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
    //ogg_int64_t nsamples=0;
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
        //nsamples+=ret;
    }
    op_free(of);

    buffer.seek(0);
    return ret==EXIT_SUCCESS;
}

bool Audio::decodeMp2k(const std::string &filePath,QByteArray &data)
{
    QFile f(QString::fromStdString(filePath));
    if(!f.open(QIODevice::ReadOnly))
    {
        std::cerr << "Audio::decodeMp2k cannot open " << filePath << std::endl;
        return false;
    }
    const QByteArray blob=f.readAll();
    f.close();
    // Render at the engine audio format (48 kHz stereo) for the same 16 s loop
    // window the opus files use; QInfiniteBuffer then loops this buffer forever.
    std::vector<float> pcm;
    if(!CatchChallenger::mp2kRenderBlob(reinterpret_cast<const uint8_t*>(blob.constData()),
                                        (size_t)blob.size(),48000,16.0,pcm))
    {
        std::cerr << "Audio::decodeMp2k render failed for " << filePath << std::endl;
        return false;
    }
    if(pcm.size()<2)
        return false;
    // Peak-normalise (matches the opus rip's loudness); NO fade — the buffer loops
    // so a fade-out would leave a gap at the loop seam.
    float peak=0.0f; std::size_t i=0;
    while(i<pcm.size()) { const float a=std::fabs(pcm[i]); if(a>peak) peak=a; ++i; }
    const float norm = peak>0.01f ? 0.85f/peak : 1.0f;
    // float -> Int16 little-endian (Audio::format()).
    data.resize((int)(pcm.size()*2));
    char *out=data.data();
    i=0;
    while(i<pcm.size())
    {
        float v=pcm[i]*norm;
        if(v>1.0f) v=1.0f; else if(v<-1.0f) v=-1.0f;
        const int16_t s=(int16_t)(v*32767.0f);
        out[i*2+0]=(char)(s&0xFF);
        out[i*2+1]=(char)((s>>8)&0xFF);
        ++i;
    }
    return true;
}

bool Audio::decodeAmbiance(const std::string &filePath,QByteArray &data)
{
    // ".mp2k" suffix -> original-GBA blob player, else the opus decoder.
    const std::string ext=".mp2k";
    if(filePath.size()>=ext.size() &&
       filePath.compare(filePath.size()-ext.size(),ext.size(),ext)==0)
        return decodeMp2k(filePath,data);
    return decodeOpus(filePath,data);
}

//if already playing ambiance then call stopCurrentAmbiance
std::string Audio::startAmbiance(const std::string &soundPath)
{
    const QString platform=QGuiApplication::platformName();
    if(platform==QLatin1String("offscreen") || platform==QLatin1String("minimal"))
        return std::string();

    stopCurrentAmbiance();

    QAudioDevice info=QMediaDevices::defaultAudioOutput();
    if (!info.isFormatSupported(m_format))
        return "raw audio format not supported by backend, cannot play audio.";

    ambiance_player = new QAudioSink(info,Audio::audio->format());
    if(ambiance_player!=nullptr)
    {
        //decode file (.mp2k original-GBA blob or opus, by extension)
        if(Audio::decodeAmbiance(soundPath,ambiance_data))
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
