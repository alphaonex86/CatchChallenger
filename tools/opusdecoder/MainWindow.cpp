#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QAudioOutput>
#include <QBuffer>
#include <QSound>
#include <iostream>

#include "opusfile/opusfile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_convert_clicked()
{
    QAudioOutput* audio;
    QAudioFormat format;
    format.setSampleRate(ui->SampleRate->value());
    format.setChannelCount(ui->ChannelCount->value());
    format.setSampleSize(ui->SampleSize->value());
    format.setCodec(ui->Codec->text());
    format.setByteOrder((QAudioFormat::Endian)ui->ByteOrder->currentIndex());
    format.setSampleType((QAudioFormat::SampleType)ui->SampleType->currentIndex());

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        ui->statusBar->showMessage("audio format not supported by backend, cannot play audio");
        std::cerr << "raw audio format not supported by backend, cannot play audio." << std::endl;
        return;
    }
    ui->statusBar->showMessage("audio format is supported by backend");
    audio = new QAudioOutput(format, this);
    buffer.open(QBuffer::ReadWrite);

    int           ret;
    OggOpusFile  *of=op_open_file("file.opus",&ret);
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

    buffer.seek(0);
    audio->start(&buffer);
}

void MainWindow::on_playDirectly_clicked()
{
    QSound(":/file.opus").play();
}
