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

#include <QAudioOutput>
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QMap>
#include <QStringList>

#include "QSoundFile.h"
#include "QSoundStream.h"
#ifdef QSOUND_USE_MAD
#include <mad.h>
#endif
#ifdef QSOUND_USE_VORBISFILE
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

static const quint32 WAV_FMT_SIZE = 16;
static const quint16 WAV_FMT_FORMAT = 1;      // linear PCM

static const quint8 MP3_ENC_LATIN1 = 0;
static const quint8 MP3_ENC_UTF8 = 3;

typedef QPair<QSoundFile::MetaData, QString> QSoundInfo;

class QSoundFilePrivate {
public:
    QSoundFilePrivate();
    virtual ~QSoundFilePrivate();
    virtual void close() = 0;
    virtual qint64 duration() const = 0;
    virtual bool open(QIODevice::OpenMode mode) = 0;
    virtual void rewind() = 0;
    virtual qint64 readData(char * data, qint64 maxSize) = 0;
    virtual qint64 writeData(const char *data, qint64 maxSize) = 0;

    QString m_name;
    QAudioFormat m_format;
    QList<QSoundInfo> m_info;

    bool m_repeat;
};

QSoundFilePrivate::QSoundFilePrivate()
    : m_repeat(false)
{
}

QSoundFilePrivate::~QSoundFilePrivate()
{
}

#ifdef QSOUND_USE_MAD
class QSoundFileMp3 : public QSoundFilePrivate
{
public:
    QSoundFileMp3(QIODevice *device, QSoundFile *qq);
    void close();
    qint64 duration() const;
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    QSoundFile *q;

    bool decodeFrame();
    bool readHeader();
    bool writeHeader();

    qint64 m_duration;
    QIODevice *m_file;
    bool m_headerFound;
    QByteArray m_inputBuffer;
    QByteArray m_outputBuffer;

    mad_stream m_stream;
    mad_frame m_frame;
    mad_synth m_synth;
    QMap<QByteArray, QSoundFile::MetaData> m_tagMap;
};

static inline qint16 mad_scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

QSoundFileMp3::QSoundFileMp3(QIODevice *device, QSoundFile *qq)
    : q(qq),
    m_duration(0),
    m_file(device),
    m_headerFound(false)
{
    // tag mapping
    m_tagMap.insert("TPE1", QSoundFile::ArtistMetaData);
    m_tagMap.insert("TALB", QSoundFile::AlbumMetaData);
    m_tagMap.insert("TIT2", QSoundFile::TitleMetaData);
    m_tagMap.insert("TYER", QSoundFile::DateMetaData);
    m_tagMap.insert("TCON", QSoundFile::GenreMetaData);
    m_tagMap.insert("TRCK", QSoundFile::TracknumberMetaData);
    m_tagMap.insert("COMM", QSoundFile::DescriptionMetaData);
}

void QSoundFileMp3::close()
{
    mad_synth_finish(&m_synth);
    mad_frame_finish(&m_frame);
    mad_stream_finish(&m_stream);
    m_inputBuffer.clear();
}

bool QSoundFileMp3::decodeFrame()
{
    if (mad_header_decode(&m_frame.header, &m_stream) == -1) {
        if (!MAD_RECOVERABLE(m_stream.error))
            return false;

        qWarning("Got an error while decoding MP3 header: 0x%04x (%s)\n", m_stream.error, mad_stream_errorstr(&m_stream));
        return true;
    }

    // process header
    if (!m_headerFound) {
        m_format = QSoundStream::pcmAudioFormat(
            MAD_NCHANNELS(&m_frame.header),
            m_frame.header.samplerate);
        m_headerFound = true;
    }

    // process frame
    if (mad_frame_decode(&m_frame, &m_stream) == -1) {
        if (!MAD_RECOVERABLE(m_stream.error))
            return false;

        qWarning("Got an error while decoding MP3 frame: 0x%04x (%s)\n", m_stream.error, mad_stream_errorstr(&m_stream));
        return true;
    }
    mad_synth_frame(&m_synth, &m_frame);

    // write PCM
    QDataStream output(&m_outputBuffer, QIODevice::WriteOnly);
    output.device()->seek(m_outputBuffer.size());
    output.setByteOrder(QDataStream::LittleEndian);

    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;

    /* pcm->samplerate contains the sampling frequency */
    struct mad_pcm *pcm = &m_synth.pcm;
    nchannels = pcm->channels;
    nsamples  = pcm->length;
    left_ch   = pcm->samples[0];
    right_ch  = pcm->samples[1];

    while (nsamples--) {
        output << mad_scale(*left_ch++);
        if (nchannels == 2)
            output << mad_scale(*right_ch++);
    }

    return true;
}

qint64 QSoundFileMp3::duration() const
{
    return m_duration;
}

static quint32 read_syncsafe_int(const QByteArray &ba)
{
    quint32 size = 0;
    size |= ba.at(0) << 21;
    size |= ba.at(1) << 14;
    size |= ba.at(2) << 7;
    size |= ba.at(3);
    return size;
}

static QByteArray write_syncsafe_int(quint32 size)
{
    QByteArray ba(4, 0);
    ba[0] = ((size >> 21) & 0x7f);
    ba[1] = ((size >> 14) & 0x7f);
    ba[2] = ((size >> 7) & 0x7f);
    ba[3] = (size & 0x7f);
    return ba;
}

bool QSoundFileMp3::open(QIODevice::OpenMode mode)
{
    // open file
    if (!m_file->open(mode))
        return false;

    if (mode & QIODevice::ReadOnly) {
        return readHeader();
    }
    else if (mode & QIODevice::WriteOnly) {
        return writeHeader();
    }
    return true;
}

qint64 QSoundFileMp3::readData(char * data, qint64 maxSize)
{
    // decode frames
    while (m_outputBuffer.size() < maxSize) {
        if (!decodeFrame())
            break;
    }

    // copy output data
    qint64 bytes = qMin(maxSize, (qint64)m_outputBuffer.size());
    if (bytes > 0) {
        memcpy(data, m_outputBuffer.constData(), bytes);
        m_outputBuffer = m_outputBuffer.mid(bytes);
    }

    return bytes;
}

bool QSoundFileMp3::readHeader()
{
    // read file contents
    m_inputBuffer = m_file->readAll();
    m_file->close();

    // handle ID3 header
    qint64 pos = 0;
    if (m_inputBuffer.startsWith("ID3")) {
        // identifier + version
        pos += 5;

        // flags
#if 0
        const quint8 flags = (quint8)(m_inputBuffer.at(pos));
        qDebug("flags %x", flags);
        if (flags & 0x80) {
            qDebug("unsynchronisation");
        }
        if (flags & 0x40) {
            qDebug("extended header");
        }
        if (flags & 0x10) {
            qDebug("footer present");
        }
#endif
        pos++;

        // size
        const quint32 size = read_syncsafe_int(m_inputBuffer.mid(pos, 4));
        pos += 4;

        // contents
        QByteArray contents = m_inputBuffer.mid(pos, size);
        qint64 ptr = 0;
        while (ptr < contents.size()) {
            // frame header
            const QByteArray frameId = contents.mid(ptr, 4);
            ptr += 4;
            const quint32 frameSize = read_syncsafe_int(contents.mid(ptr, 4));
            ptr += 4;
            // const quint16 flags = ..
            ptr += 2;
            if (!frameSize)
                break;

            // frame content
            const quint8 enc = contents.at(ptr);
            const QByteArray ba = contents.mid(ptr+1, frameSize - 1);
            ptr += frameSize;

            QString value;
            switch (enc) {
            case MP3_ENC_LATIN1:
                value = QString::fromLatin1(ba);
                break;
            case MP3_ENC_UTF8:
                value = QString::fromUtf8(ba);
                break;
            default:
                continue;
            }
            //qDebug("frame %s %s", frameId.constData(), qPrintable(value));

            if (m_tagMap.contains(frameId))
                m_info << qMakePair(m_tagMap.value(frameId), value);
        }
        pos += size;
        m_inputBuffer = m_inputBuffer.mid(pos);
    }

    // skip any data preceding MP3 data
    int startPos = m_inputBuffer.indexOf("\xff\xfb");
    if (startPos > 0)
        m_inputBuffer = m_inputBuffer.mid(startPos);

    // initialise decoder
    mad_stream_init(&m_stream);
    mad_frame_init(&m_frame);
    mad_synth_init(&m_synth);
    mad_stream_buffer(&m_stream, (unsigned char*)m_inputBuffer.data(), m_inputBuffer.size());

    // count samples
    qint64 samples = 0;
    if (!m_file->isSequential()) {
        while (mad_header_decode(&m_frame.header, &m_stream) != -1)
            samples += 32 * MAD_NSBSAMPLES(&m_frame.header);
        rewind();
    }

    // get first frame header
    if (!decodeFrame() || !m_headerFound)
        return false;

    m_duration = (1000 * samples) / m_format.sampleRate();

    return true;
}

void QSoundFileMp3::rewind()
{
    mad_stream_buffer(&m_stream, (unsigned char*)m_inputBuffer.data(), m_inputBuffer.size());
}

qint64 QSoundFileMp3::writeData(const char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}

bool QSoundFileMp3::writeHeader()
{
    // collect tags
    QByteArray tags;
    foreach(const QSoundInfo &info, m_info) {
        const QByteArray frameId = m_tagMap.key(info.first);
        if (!frameId.isEmpty()) {
            qDebug("tag %s %s", frameId.constData(), qPrintable(info.second));
            const QByteArray value = info.second.toUtf8();
            tags += frameId;
            tags += write_syncsafe_int(value.size() + 1);
            tags += QByteArray(2, 0x00); // flags
            tags += QByteArray(1, MP3_ENC_UTF8);
            tags += value;
        }
    }

    // identifier + version
    QDataStream stream(m_file);
    stream.writeRawData("ID3\x04\x00", 5);

    // flags
    stream << quint8(0);

    // size
    tags.prepend(write_syncsafe_int(tags.size()));
    stream.writeRawData(tags.constData(), tags.size());
    return true;
}

#endif

#ifdef QSOUND_USE_VORBISFILE
static size_t qfile_read_callback(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    QFile *file = static_cast<QFile*>(datasource);
    return file->read((char*)ptr, size * nmemb);
}

static int qfile_seek_callback(void *datasource, ogg_int64_t offset, int whence)
{
    QFile *file = static_cast<QFile*>(datasource);
    qint64 pos = offset;
    if (whence == SEEK_CUR)
        pos += file->pos();
    else if (whence == SEEK_END)
        pos += file->size();
    return file->seek(pos) ? 0 : -1;
}

static int qfile_close_callback(void *datasource)
{
    QFile *file = static_cast<QFile*>(datasource);
    file->close();
    return 0;
}

static long qfile_tell_callback(void *datasource)
{
    QFile *file = static_cast<QFile*>(datasource);
    return file->pos();
}

class QSoundFileOgg : public QSoundFilePrivate
{
public:
    QSoundFileOgg(QIODevice *device, QSoundFile *qq);
    void close();
        qint64 duration() const;
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    QSoundFile *q;
    QIODevice *m_file;
    int m_section;
    OggVorbis_File m_vf;
};

QSoundFileOgg::QSoundFileOgg(QIODevice *device, QSoundFile *qq)
    : q(qq),
    m_file(device),
    m_section(0)
{
}

void QSoundFileOgg::close()
{
    ov_clear(&m_vf);
}

qint64 QSoundFileOgg::duration() const
{
    return 0;
}

bool QSoundFileOgg::open(QIODevice::OpenMode mode)
{
    if (mode & QIODevice::WriteOnly) {
        qWarning("Writing to OGG is not supported");
        return false;
    }

    if (!m_file->open(mode))
        return false;

    ov_callbacks callbacks;
    callbacks.read_func = qfile_read_callback;
    callbacks.seek_func = qfile_seek_callback;
    callbacks.close_func = qfile_close_callback;
    callbacks.tell_func = qfile_tell_callback;
    if (ov_open_callbacks(m_file, &m_vf, 0, 0, callbacks) < 0) {
        qWarning("Input does not appear to be an Ogg bitstream");
        return false;
    }

    vorbis_info *vi = ov_info(&m_vf, -1);
    m_format = QSoundStream::pcmAudioFormat(vi->channels, vi->rate);

    return true;
}

qint64 QSoundFileOgg::readData(char * data, qint64 maxSize)
{
    char *ptr = data;
    while (maxSize) {
        qint64 bytes = ov_read(&m_vf, ptr, maxSize, 0, 2, 1, &m_section);
        if (bytes < 0)
            return -1;
        else if (!bytes)
            break;
        ptr += bytes;
        maxSize -= bytes;
    }
    return ptr - data;
}

void QSoundFileOgg::rewind()
{
    ov_raw_seek(&m_vf, 0);
}

qint64 QSoundFileOgg::writeData(const char * data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}
#endif

class QSoundFileWav : public QSoundFilePrivate
{
public:
    QSoundFileWav(QIODevice *device, QSoundFile *qq);
    void close();
    qint64 duration() const;
    bool open(QIODevice::OpenMode mode);
    void rewind();
    qint64 readData(char * data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    bool readHeader();
    bool writeHeader();

    QSoundFile *q;
    QIODevice *m_file;
    qint64 m_beginPos;
    qint64 m_endPos;
    QMap<QByteArray, QSoundFile::MetaData> m_tagMap;
};

QSoundFileWav::QSoundFileWav(QIODevice *device, QSoundFile *qq)
    : q(qq),
    m_file(device),
    m_beginPos(0),
    m_endPos(0)
{
    // tag mapping
    m_tagMap.insert("IART", QSoundFile::ArtistMetaData);
    m_tagMap.insert("INAM", QSoundFile::TitleMetaData);
    m_tagMap.insert("ICRD", QSoundFile::DateMetaData);
    m_tagMap.insert("IGNR", QSoundFile::GenreMetaData);
    m_tagMap.insert("ICMT", QSoundFile::DescriptionMetaData);
}

bool QSoundFileWav::open(QIODevice::OpenMode mode)
{
    // open file
    if (!m_file->open(mode))
        return false;

    if (mode & QIODevice::ReadOnly) {
        // read header
        if (!readHeader()) {
            m_file->close();
            return false;
        }
    }
    else if (mode & QIODevice::WriteOnly) {
        // write header
        if (!writeHeader()) {
            m_file->close();
            return false;
        }
    }
    return true;
}

void QSoundFileWav::close()
{
    if (q->openMode() & QIODevice::WriteOnly) {
        m_file->seek(0);
        writeHeader();
    }
    m_file->close();
}

qint64 QSoundFileWav::duration() const
{
    return (8000 * (m_endPos - m_beginPos)) / (m_format.sampleSize() * m_format.channelCount()  * m_format.sampleRate());
}

qint64 QSoundFileWav::readData(char * data, qint64 maxSize)
{
    return m_file->read(data, maxSize);
}

bool QSoundFileWav::readHeader()
{
    QDataStream stream(m_file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char chunkId[5];
    chunkId[4] = '\0';
    quint32 chunkSize;
    char chunkFormat[5];
    chunkFormat[4] = '\0';

    // RIFF header
    if (stream.readRawData(chunkId, 4) != 4)
        return false;
    stream >> chunkSize;

    stream.readRawData(chunkFormat, 4);
    if (qstrcmp(chunkId, "RIFF") || chunkSize != m_file->size() - 8 || qstrcmp(chunkFormat, "WAVE")) {
        qWarning("Bad RIFF header");
        return false;
    }

    // read subchunks
    m_info.clear();
    while (true) {
        if (stream.readRawData(chunkId, 4) != 4)
            break;
        stream >> chunkSize;

        //qDebug("subchunk %s (%u bytes)", chunkId, chunkSize);
        if (!qstrcmp(chunkId, "fmt ")) {
            // fmt subchunk
            quint16 audioFormat, channelCount, blockAlign, sampleSize;
            quint32 sampleRate, byteRate;
            stream >> audioFormat;
            if (chunkSize != WAV_FMT_SIZE || audioFormat != WAV_FMT_FORMAT) {
                qWarning("Bad fmt subchunk");
                return false;
            }
            stream >> channelCount;
            stream >> sampleRate;
            stream >> byteRate;
            stream >> blockAlign;
            stream >> sampleSize;

            //qDebug("channelCount: %u, sampleRate: %u, sampleSize: %u", channelCount, sampleRate, sampleSize);
            // prepare format
            m_format.setChannelCount(channelCount);
            m_format.setSampleRate(sampleRate);
            m_format.setSampleSize(sampleSize);
            m_format.setCodec("audio/pcm");
            m_format.setByteOrder(QAudioFormat::LittleEndian);
            m_format.setSampleType(QAudioFormat::SignedInt);
        } else if (!qstrcmp(chunkId, "data")) {
            // data subchunk
            m_beginPos = m_file->pos();
            m_endPos = m_beginPos + chunkSize;
            break;
        } else if (!qstrcmp(chunkId, "LIST")) {
            stream.readRawData(chunkFormat, 4);
            chunkSize -= 4;
            if (!qstrcmp(chunkFormat, "INFO")) {
                QByteArray key(4, '\0');
                QByteArray value;
                quint32 length;
                while (chunkSize) {
                    stream.readRawData(key.data(), key.size());
                    stream >> length;
                    value.resize(length);
                    stream.readRawData(value.data(), value.size());
                    int pos = value.indexOf('\0');
                    if (pos >= 0)
                        value = value.left(pos);

                    if (m_tagMap.contains(key))
                        m_info << qMakePair(m_tagMap.value(key), QString::fromUtf8(value));
                    chunkSize -= (8 + length);
                }
            } else {
                qWarning("WAV file contains an unknown LIST format %s", chunkFormat);
                stream.skipRawData(chunkSize);
            }
        } else {
            // skip unknown subchunk
            qWarning("WAV file contains unknown subchunk %s", chunkId);
            stream.skipRawData(chunkSize);
        }
    }
    return true;
}

void QSoundFileWav::rewind()
{
    m_file->seek(m_beginPos);
}

qint64 QSoundFileWav::writeData(const char * data, qint64 maxSize)
{
    qint64 bytes = m_file->write(data, maxSize);
    if (bytes > 0)
        m_endPos += bytes;
    return bytes;
}

bool QSoundFileWav::writeHeader()
{
    QDataStream stream(m_file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // RIFF header
    stream.writeRawData("RIFF", 4);
    stream << quint32(m_endPos - 8);
    stream.writeRawData("WAVE", 4);

    // fmt subchunk
    stream.writeRawData("fmt ", 4);
    stream << WAV_FMT_SIZE;
    stream << WAV_FMT_FORMAT;
    stream << quint16(m_format.channelCount());
    stream << quint32(m_format.sampleRate());
    stream << quint32((m_format.channelCount() * m_format.sampleSize() * m_format.sampleRate()) / 8);
    stream << quint16((m_format.channelCount() * m_format.sampleSize()) / 8);
    stream << quint16(m_format.sampleSize());

    // LIST subchunk
    if (!m_info.isEmpty()) {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QDataStream tmp(&buffer);
        tmp.setByteOrder(QDataStream::LittleEndian);
        QPair<QSoundFile::MetaData, QString> info;
        foreach (info, m_info) {
            const QByteArray key = m_tagMap.key(info.first);
            if (key.isEmpty())
                continue;
            tmp.writeRawData(key.constData(), key.size());
            QByteArray value = info.second.toUtf8() + '\0';
            if (value.size() % 2)
                value += '\0';
            tmp << quint32(value.size());
            tmp.writeRawData(value.constData(), value.size());
        }
        const QByteArray data = buffer.data();
        stream.writeRawData("LIST", 4);
        stream << quint32(4 + data.size());
        stream.writeRawData("INFO", 4);
        stream.writeRawData(data.constData(), data.size());
    }
    // data subchunk
    stream.writeRawData("data", 4);
    if (m_beginPos != m_file->pos() + 4)
        m_beginPos = m_endPos = m_file->pos() + 4;
    stream << quint32(m_endPos - m_beginPos);

    return true;
}

static QSoundFilePrivate *factory(QIODevice *file, QSoundFile::FileType type, QSoundFile *parent)
{
    switch (type) {
    case QSoundFile::WavFile:
        return new QSoundFileWav(file, parent);
#ifdef QSOUND_USE_MAD
    case QSoundFile::Mp3File:
        return new QSoundFileMp3(file, parent);
#endif
#ifdef QSOUND_USE_VORBISFILE
    case QSoundFile::OggFile:
        return new QSoundFileOgg(file, parent);
#endif
    default:
        return 0;
    }
}

/** Constructs a QSoundFile.
 *
 * @param file
 * @param type
 * @param parent
 */
QSoundFile::QSoundFile(QIODevice *file, FileType type, QObject *parent)
    : QIODevice(parent)
{
    file->setParent(this);
    d = factory(file, type, this);
}

/** Constructs a QSoundFile to represent the sound file with the given name.
 *
 * @param name
 * @param parent
 */
QSoundFile::QSoundFile(const QString &name, QObject *parent)
    : QIODevice(parent)
{
    QFile *file = new QFile(name, this);
    d = factory(file, typeFromFileName(name), this);
    if (d)
        d->m_name = name;
}

/** Destroys the sound file object.
 */
QSoundFile::~QSoundFile()
{
    if (d)
        delete d;
}

/** Closes the sound file.
 */
void QSoundFile::close()
{
    if (!isOpen())
        return;

    d->close();
    QIODevice::close();
}

/** Returns the duration of the sound file in milliseconds.
 */
qint64 QSoundFile::duration() const
{
    if (d)
        return d->duration();
    else
        return 0;
}

/** Returns the sound file format.
 */
QAudioFormat QSoundFile::format() const
{
    if (d)
        return d->m_format;
    else
        return QAudioFormat();
}

/** Sets the sound file format.
 *
 * @param format
 */
void QSoundFile::setFormat(const QAudioFormat &format)
{
    if (d)
        d->m_format = format;
}

/** Guesses the file type from a file name.
 *
 * @param fileName
 */
QSoundFile::FileType QSoundFile::typeFromFileName(const QString &name)
{
    if (name.endsWith(".wav"))
        return QSoundFile::WavFile;
    else if (name.endsWith(".mp3"))
        return QSoundFile::Mp3File;
    else if (name.endsWith(".ogg"))
        return QSoundFile::OggFile;
    else
        return QSoundFile::UnknownFile;
}


/** Guesses the file type from a MIME type.
 *
 * @param mimeType
 */
QSoundFile::FileType QSoundFile::typeFromMimeType(const QByteArray &mimeType)
{
    if (mimeType == "audio/mpeg")
        return QSoundFile::Mp3File;
    else if (mimeType == "audio/ogg" ||
             mimeType == "application/ogg")
        return QSoundFile::OggFile;
    else if (mimeType == "audio/vnd.wave" ||
             mimeType == "audio/wav" ||
             mimeType == "audio/wave" ||
             mimeType == "audio/x-wav")
        return QSoundFile::WavFile;
    else
        return QSoundFile::UnknownFile;
}

QStringList QSoundFile::metaData(MetaData key) const
{
    QStringList values;
    if (d) {
        for (int i = 0; i < d->m_info.size(); ++i) {
            if (d->m_info[i].first == key)
                values << d->m_info[i].second;
        }
    }
    return values;
}

/** Returns the sound file meta-data.
 */
QList<QPair<QSoundFile::MetaData, QString> > QSoundFile::metaData() const
{
    if (d)
        return d->m_info;
    else
        return QList<QPair<QSoundFile::MetaData, QString> >();
}

/** Sets the sound file meta-data.
 *
 * @param info
 */
void QSoundFile::setMetaData(const QList<QPair<QSoundFile::MetaData, QString> > &info)
{
    if (d)
        d->m_info = info;
}

bool QSoundFile::open(QIODevice::OpenMode mode)
{
    if (!d) {
        qWarning("Unsupported file type");
        return false;
    }

    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        qWarning("Cannot open in read/write mode");
        return false;
    }

    // open file
    if (!d->open(mode)) {
        qWarning("Could not open sound file %s", qPrintable(d->m_name));
        return false;
    }

    return QIODevice::open(mode);
}

qint64 QSoundFile::readData(char * data, qint64 maxSize)
{
    char *start = data;

    while (maxSize) {
        qint64 bytes = d->readData(data, maxSize);
        if (bytes < 0)
            return -1;
        else if (!bytes && !pos())
            break;
        data += bytes;
        maxSize -= bytes;

        if (maxSize) {
            if (!d->m_repeat)
                break;
            d->rewind();
        }
    }
    return data - start;
}

/** Returns true if the file should be read repeatedly.
 */
bool QSoundFile::repeat() const
{
    if (d)
        return d->m_repeat;
    else
        return false;
}

/** Set to true if the file should be read repeatedly.
 *
 * @param repeat
 */
void QSoundFile::setRepeat(bool repeat)
{
    if (d)
        d->m_repeat = repeat;
}

qint64 QSoundFile::writeData(const char * data, qint64 maxSize)
{
    return d->writeData(data, maxSize);
}
