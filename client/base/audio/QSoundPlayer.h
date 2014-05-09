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

#ifndef __WILINK_SOUND_PLAYER_H__
#define __WILINK_SOUND_PLAYER_H__

#include <QStringList>
#include <QUrl>

class QAudioDeviceInfo;
class QNetworkAccessManager;
class QSoundFile;
class QSoundPlayer;
class QSoundPlayerPrivate;

class QSoundPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString inputDeviceName READ inputDeviceName WRITE setInputDeviceName NOTIFY inputDeviceNameChanged)
    Q_PROPERTY(QString outputDeviceName READ outputDeviceName WRITE setOutputDeviceName NOTIFY outputDeviceNameChanged)
    Q_PROPERTY(QStringList inputDeviceNames READ inputDeviceNames CONSTANT)
    Q_PROPERTY(QStringList outputDeviceNames READ outputDeviceNames CONSTANT)

public:
    QSoundPlayer(QObject *parent = 0);
    ~QSoundPlayer();

    static QSoundPlayer *instance();

    QString inputDeviceName() const;
    void setInputDeviceName(const QString &name);

    QString outputDeviceName() const;
    void setOutputDeviceName(const QString &name);

    QAudioDeviceInfo inputDevice() const;
    QStringList inputDeviceNames() const;

    QAudioDeviceInfo outputDevice() const;
    QStringList outputDeviceNames() const;

    QThread *soundThread() const;

signals:
    void inputDeviceNameChanged();
    void outputDeviceNameChanged();

private:
    QSoundPlayerPrivate *d;
};

#endif
