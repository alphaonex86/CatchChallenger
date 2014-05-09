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

#ifndef __WILINK_SOUND_METER_H__
#define __WILINK_SOUND_METER_H__

#include <QIODevice>

class QAudioFormat;

/** The QSoundMeter class acts as a proxy to a QIODevice which evaluates the
 *  sound level as samples are read or written.
 */
class QSoundMeter : public QIODevice
{
    Q_OBJECT

public:
    QSoundMeter(const QAudioFormat &format, QIODevice *device, QObject *parent = 0);
    static int maximum();
    qint64 pos() const;
    bool seek(qint64 pos);
    int value() const;

signals:
    void valueChanged(int value);

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private slots:
    void _q_deviceDestroyed(QObject *obj);
    void _q_emitSignals();

private:
    QIODevice *m_device;
    qint64 m_pos;
    int m_sampleSize;
    bool m_signalsQueued;
    int m_value;
};

#endif
