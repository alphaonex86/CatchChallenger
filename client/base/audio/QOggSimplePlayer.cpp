#include "QOggSimplePlayer.h"

#include <QDebug>
#include <QDir>

#define MAX_BUFFER_SIZE 64*1024
#define MIN_BUFFER_SIZE 16*1024

QOggSimplePlayer::QOggSimplePlayer(const QString &filePath, QThread *audioThread)
{
    if(audioThread!=NULL)
        moveToThread(audioThread);
    needPlay=false;
    volume=0;
    output=NULL;
    loop=false;
    buffer.open(QIODevice::ReadWrite|QIODevice::Unbuffered);
    this->filePath=filePath;

    connect(&buffer,&QOggAudioBuffer::readDone,this,&QOggSimplePlayer::readDone,Qt::QueuedConnection);
    connect(this,&QOggSimplePlayer::internalOpen,this,&QOggSimplePlayer::open,Qt::QueuedConnection);
    if(audioThread!=NULL)
        connect(this,&QOggSimplePlayer::internalClose,this,&QOggSimplePlayer::close,Qt::BlockingQueuedConnection);
    else
        connect(this,&QOggSimplePlayer::internalClose,this,&QOggSimplePlayer::close,Qt::DirectConnection);
    emit internalOpen();
}

QOggSimplePlayer::~QOggSimplePlayer()
{
    emit internalClose();
}

void QOggSimplePlayer::setVolume(qreal volume)
{
    this->volume=volume;
    if(output!=NULL)
        output->setVolume(volume);
}

QString QOggSimplePlayer::getFilePath() const
{
    return filePath;
}

void QOggSimplePlayer::open()
{
    if(!QFile(filePath).exists())
        return;
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    int returnCode=ov_fopen(QDir::toNativeSeparators(filePath).toLocal8Bit().data(), &vf);
    if(returnCode < 0)
    {
        qDebug() << "Input does not appear to be an Ogg bitstream for " << QDir::toNativeSeparators(filePath) << ", return code: " << returnCode;
        ov_clear(&vf);
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
        format.setChannelCount(vi->channels);
        format.setSampleRate(vi->rate);
    }

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qDebug() << QString("raw audio format not supported by backend, cannot play audio: SampleRate: %1, ChannelCount: %2, SampleSize: %3, Codec: %4, ByteOrder: %5, SampleType: %6")
                    .arg(format.sampleRate())
                    .arg(format.channelCount())
                    .arg(format.sampleSize())
                    .arg(format.codec())
                    .arg(format.byteOrder())
                    .arg(format.sampleType())
                    ;
        QAudioFormat preferedFormat=info.preferredFormat();
        qDebug() << QString("prefered format: SampleRate: %1, ChannelCount: %2, SampleSize: %3, Codec: %4, ByteOrder: %5, SampleType: %6")
                    .arg(preferedFormat.sampleRate())
                    .arg(preferedFormat.channelCount())
                    .arg(preferedFormat.sampleSize())
                    .arg(preferedFormat.codec())
                    .arg(preferedFormat.byteOrder())
                    .arg(preferedFormat.sampleType())
                    ;
        QAudioFormat nearestFormat=info.nearestFormat(format);
        qDebug() << QString("nearest format: SampleRate: %1, ChannelCount: %2, SampleSize: %3, Codec: %4, ByteOrder: %5, SampleType: %6")
                    .arg(nearestFormat.sampleRate())
                    .arg(nearestFormat.channelCount())
                    .arg(nearestFormat.sampleSize())
                    .arg(nearestFormat.codec())
                    .arg(nearestFormat.byteOrder())
                    .arg(nearestFormat.sampleType())
                    ;
        ov_clear(&vf);
        return;
    }

    if(output!=NULL)
        delete output;
    output = new QAudioOutput(format);
    output->setVolume(volume);
    output->setBufferSize(4096);
    connect(output,&QAudioOutput::stateChanged,this,&QOggSimplePlayer::finishedPlaying,Qt::QueuedConnection);

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
        ov_clear(&vf);
    }
    buffer.close();
    /*do by ov_clear
    if(file!=NULL)
    {
        fclose(file);
        file=NULL;
    }*/
}

void QOggSimplePlayer::finishedPlaying(QAudio::State state)
{
    if (state == QAudio::IdleState)
    {
        stop();
        if(loop)
            start();
    }
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
}

void QOggSimplePlayer::setLoop(const bool &loop)
{
    this->loop=loop;
}

void QOggSimplePlayer::readDone()
{
    if(output==NULL)
        return;
    if(buffer.data().size()<MIN_BUFFER_SIZE)
    {
        char pcmout[4096];
        buffer.seek(buffer.data().size());
        while(buffer.data().size()<MAX_BUFFER_SIZE){
            long ret=ov_read(&vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
            if (ret == 0) {
                /* EOF */
                break;
            } else if (ret < 0) {
                /* error in the stream.  Not a problem, just reporting it in
                case we (the app) cares.  In this case, we don't. */
            } else {
                /* we don't bother dealing with sample rate changes, etc, but
                you'll have to */
                QByteArray tempData(pcmout,ret);
                //int pos=buffer.pos();
                if(tempData.isEmpty())
                    break;
                buffer.write(tempData);
                //buffer.seek(pos);
            }
        }
        buffer.seek(0);
    }
}
