#include "GameLoaderThread.h"

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../opusfile/opusfile.h"
#include <QBuffer>
#endif
#include <QDirIterator>
#include <iostream>

GameLoaderThread::GameLoaderThread()
{
}

void GameLoaderThread::run()
{
    unsigned int index=0;
    while(index<toLoad.size())
    {
        const QString &file=toLoad.at(index);
        if(file.endsWith(QStringLiteral(".opus"))) {
            #ifndef CATCHCHALLENGER_NOAUDIO
            QBuffer buffer;
            buffer.open(QBuffer::ReadWrite);
            QFile f(file);
            if(!f.open(QFile::ReadOnly))
            {
                index++;
                continue;
            }
            const QByteArray data=f.readAll();
            f.close();

            int           ret;
            OggOpusFile  *of=op_open_memory(reinterpret_cast<const unsigned char *>(data.constData()),data.size(),&ret);
            if(of==NULL) {
                fprintf(stderr,"Failed to open file '%s': %i\n",file.toStdString().c_str(),ret);
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
            {
                //fprintf(stderr,"\nDone: played ");
                musics[file]=buffer.data();
            }
            op_free(of);
            emit addSize(QFileInfo(file).size());
            #endif
        }
        else if(file.endsWith(QStringLiteral(".png")) ||
                file.endsWith(QStringLiteral(".jpg")) ||
                file.endsWith(QStringLiteral(".webp"))) {
            images[file]=QImage(file);
            emit addSize(QFileInfo(file).size());
        }
        else {
            std::cerr << "File format not supoprted" << std::endl;
            abort();
        }
        index++;
    }
}
