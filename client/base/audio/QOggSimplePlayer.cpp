#include "QOggSimplePlayer.h"

#include <QDebug>

#define MAX_BUFFER_SIZE 64*1024
#define MIN_BUFFER_SIZE 16*1024

QOggSimplePlayer::QOggSimplePlayer(const QString &filePath, QThread *audioThread)
{
    if(audioThread!=NULL)
        moveToThread(audioThread);
    needPlay=false;
    output=NULL;
    file=NULL;
    buffer.open(QIODevice::ReadWrite|QIODevice::Unbuffered);
    this->filePath=filePath;

    connect(&buffer,SIGNAL(readDone()),SLOT(readDone()));
    connect(this,SIGNAL(internalOpen()),SLOT(open()),Qt::QueuedConnection);
    if(audioThread!=NULL)
        connect(this,SIGNAL(internalClose()),SLOT(close()),Qt::BlockingQueuedConnection);
    else
        connect(this,SIGNAL(internalClose()),SLOT(close()),Qt::DirectConnection);
    emit internalOpen();
}

QOggSimplePlayer::~QOggSimplePlayer()
{
    emit internalClose();
}

QString QOggSimplePlayer::getFilePath() const
{
    return filePath;
}

void QOggSimplePlayer::open()
{
    QAudioFormat format;
    format.setFrequency(48000);
    format.setChannels(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if(file!=NULL)
    {
        fclose(file);
        delete file;
    }
    file=fopen(filePath.toLocal8Bit().data(),"r");
    if(file==NULL)
    {
        qDebug() << "Unable to open the file";
        return;
    }

    if(ov_open_callbacks(/*stdin*/file, &vf, NULL, 0, OV_CALLBACKS_STREAMONLY) < 0)
    {
        qDebug() << "Input does not appear to be an Ogg bitstream";
        return;
    }

    {
        char **ptr=ov_comment(&vf,-1)->user_comments;
        vorbis_info *vi=ov_info(&vf,-1);
        while(*ptr){
            fprintf(stderr,"%s\n",*ptr);
            ++ptr;
        }
        qDebug() << QString("Bitstream is %1 channel, %2Hz").arg(vi->channels).arg(vi->rate);
        qDebug() << QString("Encoded by: %1").arg(ov_comment(&vf,-1)->vendor);
        format.setChannels(vi->channels);
        format.setFrequency(vi->rate);
        format.setSampleRate(vi->rate);
    }

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qDebug() << "raw audio format not supported by backend, cannot play audio.";
        return;
    }

    if(output!=NULL)
        delete output;
    output = new QAudioOutput(format);
    output->setBufferSize(4096);
    connect(output,SIGNAL(stateChanged(QAudio::State)),SLOT(finishedPlaying(QAudio::State)),Qt::QueuedConnection);

    if(needPlay)
        start();
}

void QOggSimplePlayer::close()
{
    if(output!=NULL)
    {
        output->stop();
        delete output;
        output=NULL;
    }
    buffer.close();
    /*do by ov_clear
    if(file!=NULL)
    {
        fclose(file);
        file=NULL;
    }*/
    ov_clear(&vf);
}

void QOggSimplePlayer::finishedPlaying(QAudio::State state)
{
    if (state == QAudio::IdleState)
        stop();
}

void QOggSimplePlayer::start()
{
    needPlay=true;
    if(output==NULL)
        return;
    stop();
    readDone();
    output->start(&buffer);
}

void QOggSimplePlayer::stop()
{
    needPlay=false;
    if(output==NULL)
        return;
    output->reset();
    buffer.seek(0);
    buffer.close();
    buffer.setData(QByteArray());
    buffer.open(QIODevice::ReadWrite|QIODevice::Unbuffered);
    current_section=0;
    ov_time_seek(&vf,0);
    fseek(file,0,SEEK_SET);
}

void QOggSimplePlayer::readDone()
{
    if(output==NULL)
        return;
    if(buffer.data().size()<MIN_BUFFER_SIZE)
    {
        char pcmout[4096];
        bool eof=false;
        buffer.seek(buffer.data().size());
        while(!eof && buffer.data().size()<MAX_BUFFER_SIZE){
            long ret=ov_read(&vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
            if (ret == 0) {
                /* EOF */
                eof=true;
            } else if (ret < 0) {
                /* error in the stream.  Not a problem, just reporting it in
                case we (the app) cares.  In this case, we don't. */
            } else {
                /* we don't bother dealing with sample rate changes, etc, but
                you'll have to */
                QByteArray tempData(pcmout,ret);
                //int pos=buffer.pos();
                buffer.write(tempData);
                //buffer.seek(pos);
            }
        }
        buffer.seek(0);
    }
}
