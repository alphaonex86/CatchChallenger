/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
 * See AUTHORS file for a full list of contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QAudioInput>
#include <QAudioOutput>
#include <QTime>

#include "QSoundMeter.h"
#include "QSoundPlayer.h"
#include "QSoundStream.h"

static int bufferFor(const QAudioFormat &format)
{
#ifdef Q_OS_MAC
    // 128ms at 8kHz
    return 2048 * format.channelCount();
#else
    // 160ms at 8kHz
    return 2560 * format.channelCount();
#endif
}

class QSoundStreamPrivate
{
public:
    QSoundStreamPrivate();

    QAudioFormat audioFormat;
    QAudioInput *audioInput;
    QSoundMeter *audioInputMeter;
    QAudioOutput *audioOutput;
    QSoundMeter *audioOutputMeter;
    QIODevice *device;
    QSoundPlayer *soundPlayer;
};

QSoundStreamPrivate::QSoundStreamPrivate()
    : audioInput(0),
    audioInputMeter(0),
    audioOutput(0),
    audioOutputMeter(0),
    device(0),
    soundPlayer(0)
{
}

QSoundStream::QSoundStream(QSoundPlayer *player)
{
    d = new QSoundStreamPrivate;
    d->soundPlayer = player;
}

QSoundStream::~QSoundStream()
{
    delete d;
}

QIODevice* QSoundStream::device() const
{
    return d->device;
}

void QSoundStream::setDevice(QIODevice *device)
{
    d->device = device;
}

QAudioFormat QSoundStream::format() const
{
    return d->audioFormat;
}

void QSoundStream::setFormat(unsigned char channels, unsigned int clockrate)
{
    d->audioFormat = pcmAudioFormat(channels, clockrate);
}

QAudioFormat QSoundStream::pcmAudioFormat(unsigned char channels, unsigned int clockrate)
{
    QAudioFormat format;
    format.setChannelCount(channels);
    format.setSampleRate(clockrate);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    return format;
}

/** Starts audio capture.
 */
void QSoundStream::startInput()
{
    bool check;
    Q_UNUSED(check);
    Q_ASSERT(d->device);

    if (!d->audioInput) {
        const int bufferSize = bufferFor(d->audioFormat);

        QTime tm;
        tm.start();

        d->audioInput = new QAudioInput(d->soundPlayer->inputDevice(), d->audioFormat, this);
        check = connect(d->audioInput, SIGNAL(stateChanged(QAudio::State)),
                        this, SLOT(_q_audioInputStateChanged()));
        Q_ASSERT(check);

        d->audioInputMeter = new QSoundMeter(d->audioFormat, d->device, this);
        check = connect(d->audioInputMeter, SIGNAL(valueChanged(int)),
                        this, SIGNAL(inputVolumeChanged(int)));
        Q_ASSERT(check);

        d->audioInput->setBufferSize(bufferSize);
        d->audioInput->start(d->audioInputMeter);

        qDebug("QSoundStream audio input initialized in %i ms", tm.elapsed());
        qDebug("QSoundStream audio input buffer size %i (asked for %i)", d->audioInput->bufferSize(), bufferSize);
    }
}

/** Stops audio capture.
 */
void QSoundStream::stopInput()
{
    if (d->audioInput) {
        d->audioInput->stop();
        delete d->audioInput;
        d->audioInput = 0;
        delete d->audioInputMeter;
        d->audioInputMeter = 0;

        qDebug("QSoundStream audio input stopped");

        emit inputVolumeChanged(0);
    }
}

/** Starts audio playback.
 */
void QSoundStream::startOutput()
{
    bool check;
    Q_UNUSED(check);
    Q_ASSERT(d->device);

    if (!d->audioOutput) {
        const int bufferSize = bufferFor(d->audioFormat);

        QTime tm;
        tm.start();

        d->audioOutput = new QAudioOutput(d->soundPlayer->outputDevice(), d->audioFormat, this);
        check = connect(d->audioOutput, SIGNAL(stateChanged(QAudio::State)),
                        this, SLOT(_q_audioOutputStateChanged()));
        Q_ASSERT(check);

        d->audioOutputMeter = new QSoundMeter(d->audioFormat, d->device, this);
        check = connect(d->audioOutputMeter, SIGNAL(valueChanged(int)),
                        this, SIGNAL(outputVolumeChanged(int)));
        Q_ASSERT(check);

        d->audioOutput->setBufferSize(bufferSize);
        d->audioOutput->start(d->audioOutputMeter);

        qDebug("QSoundStream audio output initialized in %i ms", tm.elapsed());
        qDebug("QSoundStream audio output buffer size %i (asked for %i)", d->audioOutput->bufferSize(), bufferSize);
    }
}

/** Stops audio playback.
 */
void QSoundStream::stopOutput()
{
    if (d->audioOutput) {
        d->audioOutput->stop();
        delete d->audioOutput;
        d->audioOutput = 0;
        delete d->audioOutputMeter;
        d->audioOutputMeter = 0;

        qDebug("QSoundStream audio output stopped");

        emit outputVolumeChanged(0);
    }
}

int QSoundStream::inputVolume() const
{
    return d->audioInputMeter ? d->audioInputMeter->value() : 0;
}

int QSoundStream::maximumVolume() const
{
    return QSoundMeter::maximum();
}

int QSoundStream::outputVolume() const
{
    return d->audioOutputMeter ? d->audioOutputMeter->value() : 0;
}

void QSoundStream::_q_audioInputStateChanged()
{
#if 0
    qDebug("Audio input state %i error %i",
           d->audioInput->state(),
           d->audioInput->error());
#endif

    // Restart audio input if we get an underrun.
    //
    // NOTE: seen on Mac OS X 10.6
    if (d->audioInput->state() == QAudio::IdleState &&
            d->audioInput->error() == QAudio::UnderrunError) {
        qWarning("QSoundStream audio input needs restart due to buffer underrun");
        d->audioInput->start(d->audioInputMeter);
    }
}

void QSoundStream::_q_audioOutputStateChanged()
{
#if 0
    qDebug("Audio output state %i error %i",
           d->audioOutput->state(),
           d->audioOutput->error());
#endif

    // Restart audio output if we get an underrun.
    //
    // NOTE: seen on Linux with pulseaudio
    if (d->audioOutput->state() == QAudio::IdleState &&
            d->audioOutput->error() == QAudio::UnderrunError) {
        qWarning("QSoundStream audio output needs restart due to buffer underrun");
        d->audioOutput->start(d->audioOutputMeter);
    }
}