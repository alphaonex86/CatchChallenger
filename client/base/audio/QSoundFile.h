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

#ifndef __WILINK_SOUND_FILE_H__
#define __WILINK_SOUND_FILE_H__

#include <QAudioOutput>
#include <QIODevice>
#include <QPair>

class QSoundFilePrivate;

/** The QSoundFile class represents a WAV-format file.
 */
class QSoundFile : public QIODevice
{
    Q_OBJECT

public:
    enum FileType {
        UnknownFile = 0,
        OggFile,
        Mp3File,
        WavFile,
    };

    enum MetaData {
        ArtistMetaData = 0,
        AlbumMetaData,
        TitleMetaData,
        DateMetaData,
        GenreMetaData,
        TracknumberMetaData,
        DescriptionMetaData,
    };

    QSoundFile(QIODevice *file, FileType type, QObject *parent = 0);
    QSoundFile(const QString &name, QObject *parent = 0);
    ~QSoundFile();
    void close();
    qint64 duration() const;
    QAudioFormat format() const;
    void setFormat(const QAudioFormat &format);
    QStringList metaData(MetaData key) const;
    QList<QPair<MetaData, QString> > metaData() const;
    void setMetaData(const QList<QPair<MetaData, QString> > &info);
    bool open(QIODevice::OpenMode mode);
    bool repeat() const;
    void setRepeat(bool repeat);

    static FileType typeFromFileName(const QString &fileName);
    static FileType typeFromMimeType(const QByteArray &mimeType);

protected:
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char * data, qint64 maxSize);

private:
    QSoundFilePrivate *d;
};

#endif
