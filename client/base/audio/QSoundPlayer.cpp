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

#include <QAbstractNetworkCache>
#include <QAudioOutput>
#include <QIODevice>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QThread>
#include <QUrl>
#include <QVariant>

#include "QSoundFile.h"
#include "QSoundPlayer.h"

static QSoundPlayer *thePlayer = 0;

class QSoundPlayerPrivate
{
public:
    QSoundPlayerPrivate();
    QString inputName;
    QString outputName;
    QThread *soundThread;
};

QSoundPlayerPrivate::QSoundPlayerPrivate()
    : soundThread(0)
{
}

QSoundPlayer::QSoundPlayer(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QAudioDeviceInfo>();
    d = new QSoundPlayerPrivate;
    d->soundThread = new QThread(this);
    d->soundThread->start();

    if (!thePlayer)
        thePlayer = this;
}

QSoundPlayer::~QSoundPlayer()
{
    d->soundThread->quit();
    d->soundThread->wait();

    if (thePlayer == this)
        thePlayer = 0;
    delete d;
}

QSoundPlayer* QSoundPlayer::instance()
{
    return thePlayer;
}

QAudioDeviceInfo QSoundPlayer::inputDevice() const
{
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == d->inputName)
            return info;
    }
    return QAudioDeviceInfo::defaultInputDevice();
}

QString QSoundPlayer::inputDeviceName() const
{
    return d->inputName;
}

void QSoundPlayer::setInputDeviceName(const QString &name)
{
    if (name != d->inputName) {
        d->inputName = name;
        emit inputDeviceNameChanged();
    }
}

QStringList QSoundPlayer::inputDeviceNames() const
{
    QStringList names;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        names << info.deviceName();
    return names;
}

QAudioDeviceInfo QSoundPlayer::outputDevice() const
{
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.deviceName() == d->outputName)
            return info;
    }
    return QAudioDeviceInfo::defaultOutputDevice();
}

QString QSoundPlayer::outputDeviceName() const
{
    return d->outputName;
}

void QSoundPlayer::setOutputDeviceName(const QString &name)
{
    if (name != d->outputName) {
        d->outputName = name;
        emit outputDeviceNameChanged();
    }
}

QStringList QSoundPlayer::outputDeviceNames() const
{
    QStringList names;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        names << info.deviceName();
    return names;
}

QThread *QSoundPlayer::soundThread() const
{
    return d->soundThread;
}
