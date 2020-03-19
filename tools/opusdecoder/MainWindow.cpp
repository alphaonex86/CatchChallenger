#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QAudioOutput>
#include <QBuffer>
#include <QSound>
#include <QFile>
#include <iostream>

#include "opusfile/opusfile.h"

#define MAX_SAMPLE_LENGTH 16*1024*1024

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString lastok;
    QAudioFormat format;
    /*format.setSampleRate(ui->SampleRate->value());//48000
    format.setChannelCount(ui->ChannelCount->value());//2
    format.setSampleSize(ui->SampleSize->value());//16
    format.setCodec("audio/pcm");//audio/pcm
    format.setByteOrder((QAudioFormat::Endian)ui->ByteOrder->currentIndex());//LittleEndian
    format.setSampleType((QAudioFormat::SampleType)ui->SampleType->currentIndex());//SignedInt*/

    for (const char * codec : { "audio/pcm","audio/x-raw" })
    for (int SampleRate : { 22050,32000,44100,48000 })
        for (int ChannelCount : { 1,2 })
            for (int SampleSize : { 24,8,16 })
                for (QAudioFormat::Endian ByteOrder : { QAudioFormat::Endian::BigEndian,QAudioFormat::Endian::LittleEndian })
                    for (QAudioFormat::SampleType SampleType : { QAudioFormat::SampleType::Unknown, QAudioFormat::SampleType::UnSignedInt, QAudioFormat::SampleType::Float, QAudioFormat::SampleType::SignedInt })
                    {
                        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

                        format.setCodec(codec);
                        format.setSampleRate(SampleRate);//48000
                        format.setChannelCount(ChannelCount);//2
                        format.setSampleSize(SampleSize);//16
                        format.setByteOrder(ByteOrder);//LittleEndian
                        format.setSampleType(SampleType);//SignedInt

                        if(info.isFormatSupported(format))
                            lastok=QString::number(SampleRate)
                                    +","+QString::number(ChannelCount)
                                    +","+QString::number(SampleSize)
                                    +","+QString::number(ByteOrder)
                                    +","+QString::number(SampleType);
                    }

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    ui->codeclist->setText(info.supportedCodecs().join(";")+": "+lastok);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_convert_clicked()
{
    QAudioOutput* audio;
    QAudioFormat format;
    format.setSampleRate(ui->SampleRate->value());//48000
    format.setChannelCount(ui->ChannelCount->value());//2
    format.setSampleSize(ui->SampleSize->value());//16
    format.setCodec(ui->Codec->text());//audio/pcm
    format.setByteOrder((QAudioFormat::Endian)ui->ByteOrder->currentIndex());//LittleEndian
    format.setSampleType((QAudioFormat::SampleType)ui->SampleType->currentIndex());//SignedInt

    const QAudioDeviceInfo &info=QAudioDeviceInfo::defaultOutputDevice();
    if (!info.isFormatSupported(format)) {
        ui->statusBar->showMessage("audio format not supported by backend, cannot play audio");
        std::cerr << "raw audio format not supported by backend, cannot play audio." << std::endl;
        ui->codeclist->setText("bad: "+ui->Codec->text()+": "+QString::number(ui->SampleRate->value())
                               +","+QString::number(ui->ChannelCount->value())
                               +","+QString::number(ui->SampleSize->value())
                               +","+QString::number(ui->ByteOrder->currentIndex())
                               +","+QString::number(ui->SampleType->currentIndex())+" on "+info.deviceName()
                               );
        return;
    }
    ui->statusBar->showMessage("audio format is supported by backend");
    audio = new QAudioOutput(format, this);
    buffer.open(QBuffer::ReadWrite);





    QFile f(":/file.opus");
    if(!f.open(QFile::ReadOnly))
        abort();
    const QByteArray data=f.readAll();
    f.close();

    int           ret;
    OggOpusFile  *of=op_open_memory(reinterpret_cast<const unsigned char *>(data.constData()),data.size(),&ret);
    if(of==NULL) {
        fprintf(stderr,"Failed to open file '%s': %i\n","file.opus",ret);
        return;
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
    if(ret==EXIT_SUCCESS)
        fprintf(stderr,"\nDone: played ");
    op_free(of);





    /*QFile f("file.opus");
    if(!f.open(QFile::ReadOnly))
        abort();
    const QByteArray data=f.readAll();
    f.close();

    OggOpusFile *of = op_open_memory(reinterpret_cast<const unsigned char *>(data.constData()),data.size(),NULL);
    if(!of)
        abort();

    int rate = 48000;
    int channels = op_channel_count(of, -1);
    if(rate <= 0 || channels <= 0)
    {
        op_free(of);
        of = NULL;
        return;
    }
    if(channels > 2 || op_link_count(of) != 1)
    {
        // We downmix multichannel to stereo as recommended by Opus specification in
        // case we are not able to handle > 2 channels.
        // We also decode chained files as stereo even if they start with a mono
        // stream, which simplifies handling of link boundaries for us.
        channels = 2;
    }

    std::vector<int16_t> decodeBuf(120 * 48000 / 1000); // 120ms (max Opus packet), 48kHz
    bool eof = false;
    while(!eof)
    {
        int framesRead = 0;
        if(channels == 2)
        {
            framesRead = op_read_stereo(of, &(decodeBuf[0]), static_cast<int>(decodeBuf.size()));
        } else if(channels == 1)
        {
            framesRead = op_read(of, &(decodeBuf[0]), static_cast<int>(decodeBuf.size()), NULL);
        }
        if(framesRead > 0)
        {
            buffer.write(reinterpret_cast<const char *>(decodeBuf.data()),framesRead * channels);
        } else if(framesRead == 0)
        {
            eof = true;
        } else if(framesRead == OP_HOLE)
        {
            // continue
        } else
        {
            // other errors are fatal, stop decoding
            eof = true;
        }
        if((buffer.size() / channels) > MAX_SAMPLE_LENGTH)
        {
            break;
        }
    }

    op_free(of);
    of = NULL;

    if(buffer.data().isEmpty())
    {
        return;
    }*/







    buffer.seek(0);
    audio->start(&buffer);
}

void MainWindow::on_playDirectly_clicked()
{
    QSound(":/file.opus").play();
}
