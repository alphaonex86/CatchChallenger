#include "Audio.hpp"
#include "PlatformMacro.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/mp2k/Gsf.hpp"
#include "../../general/mp2k/Mp2kPlayer.hpp"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QFileInfo>
#include <vector>
#include <cmath>
#include <cstdlib>
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

bool Audio::decodeGsf(const std::string &minigsfPath,QByteArray &data)
{
    // 1. minigsf -> song id + _lib (the gsflib filename, dir-relative).
    std::vector<uint8_t> mprog; std::string mtags;
    if(!CatchChallenger::gsfReadFile(minigsfPath,mprog,mtags))
        return false;
    uint32_t e=0,o=0,sz=0; const uint8_t *payload=nullptr;
    if(!CatchChallenger::gsfSplitProgram(mprog,e,o,payload,sz) || sz==0)
        return false;
    uint32_t songId=0;
    {
        uint32_t i=0;
        while(i<sz && i<4) { songId|=(uint32_t)payload[i]<<(8*i); ++i; }
    }
    const std::string lib=CatchChallenger::gsfTag(mtags,"_lib");
    if(lib.empty())
    {
        std::cerr << "Audio::decodeGsf: " << minigsfPath << " has no _lib tag" << std::endl;
        return false;
    }
    // 2. gsflib sits in the same dir as the minigsf (the _lib filename).
    const QString libPath=QFileInfo(QString::fromStdString(minigsfPath)).absolutePath()
                          + "/" + QString::fromStdString(lib);
    std::vector<uint8_t> lprog; std::string ltags;
    if(!CatchChallenger::gsfReadFile(libPath.toStdString(),lprog,ltags))
        return false;
    uint32_t le=0,lo=0,lsz=0; const uint8_t *rom=nullptr;
    if(!CatchChallenger::gsfSplitProgram(lprog,le,lo,rom,lsz) || lsz==0)
        return false;
    // 3. gSongTable: read OUR _cc_songtable tag (so we never CPU-scan).
    const std::string st=CatchChallenger::gsfTag(ltags,"_cc_songtable");
    if(st.empty())
    {
        std::cerr << "Audio::decodeGsf: gsflib " << lib << " missing _cc_songtable tag" << std::endl;
        return false;
    }
    const uint32_t songtable=(uint32_t)std::strtoul(st.c_str(),nullptr,0);
    CatchChallenger::Mp2kRomSource src(rom,lsz);
    bool ok=false;
    const uint32_t header=src.pointer((songtable-0x08000000u)+songId*8u,&ok);
    if(!ok)
    {
        std::cerr << "Audio::decodeGsf: song " << songId << " header out of range" << std::endl;
        return false;
    }
    // 4. synth at the engine audio format (48 kHz stereo) for the 16 s loop window.
    std::vector<float> pcm;
    CatchChallenger::mp2kRenderSong(src,header,48000,16.0,pcm);
    if(pcm.size()<2)
        return false;
    float peak=0.0f; std::size_t i=0;
    while(i<pcm.size()) { const float a=std::fabs(pcm[i]); if(a>peak) peak=a; ++i; }
    const float norm = peak>0.01f ? 0.85f/peak : 1.0f;
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
    const std::string ext=".minigsf";
    if(filePath.size()>=ext.size() &&
       filePath.compare(filePath.size()-ext.size(),ext.size(),ext)==0)
        return decodeGsf(filePath,data);
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
        //decode file
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
