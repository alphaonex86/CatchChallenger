import threading
import subprocess
import os
import sys
import multiprocessing
import logging

iterator = -1
logging.basicConfig(level=logging.DEBUG, format='(%(threadName)-9s) %(message)s',)
has_success = True

class Requester(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.lock = threading.Lock()

    def run(self):
        global iterator, content, length
        while True:
            self.lock.acquire()
            iterator = iterator + 1
            if iterator < length:
                command = content[iterator]
            else:
                command = None
            self.lock.release()
            if command is None:
                break
            elif len(command) > 0:
                parts = command.strip().split(' ')
                try:
                    output = subprocess.run(parts, stdout=subprocess.PIPE, check=True)
                    if output.returncode != 0:
                        logging.error(output.stderr)
                except Exception as err:
                    logging.error('command: ' + command + ' exception: ' + str(err))

content = """
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o CompressionProtocol.o ../../general/base/CompressionProtocol.cpp
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o debug.o ../../general/libzstd/lib/common/debug.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o entropy_common.o ../../general/libzstd/lib/common/entropy_common.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o error_private.o ../../general/libzstd/lib/common/error_private.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o fse_decompress.o ../../general/libzstd/lib/common/fse_decompress.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o pool.o ../../general/libzstd/lib/common/pool.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o threading.o ../../general/libzstd/lib/common/threading.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o xxbhash.o ../../general/libzstd/lib/common/xxbhash.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_common.o ../../general/libzstd/lib/common/zstd_common.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o huf_decompress.o ../../general/libzstd/lib/decompress/huf_decompress.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_ddict.o ../../general/libzstd/lib/decompress/zstd_ddict.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_decompress.o ../../general/libzstd/lib/decompress/zstd_decompress.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_decompress_block.o ../../general/libzstd/lib/decompress/zstd_decompress_block.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o fse_compress.o ../../general/libzstd/lib/compress/fse_compress.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o hist.o ../../general/libzstd/lib/compress/hist.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o huf_compress.o ../../general/libzstd/lib/compress/huf_compress.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_compress.o ../../general/libzstd/lib/compress/zstd_compress.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_compress_literals.o ../../general/libzstd/lib/compress/zstd_compress_literals.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_compress_sequences.o ../../general/libzstd/lib/compress/zstd_compress_sequences.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_compress_superblock.o ../../general/libzstd/lib/compress/zstd_compress_superblock.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_double_fast.o ../../general/libzstd/lib/compress/zstd_double_fast.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_fast.o ../../general/libzstd/lib/compress/zstd_fast.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_lazy.o ../../general/libzstd/lib/compress/zstd_lazy.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_ldm.o ../../general/libzstd/lib/compress/zstd_ldm.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstd_opt.o ../../general/libzstd/lib/compress/zstd_opt.c
clang -c -pipe -O2 -g -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o zstdmt_compress.o ../../general/libzstd/lib/compress/zstdmt_compress.c
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o sha224.o ../../general/sha224/sha224.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollClientLoginSlave.o ../gateway/EpollClientLoginSlave.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollServerLoginSlave.o ../gateway/EpollServerLoginSlave.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollClientLoginSlaveStaticVar.o ../gateway/EpollClientLoginSlaveStaticVar.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollClientLoginSlaveProtocolParsing.o ../gateway/EpollClientLoginSlaveProtocolParsing.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollClientLoginSlaveWrite.o ../gateway/EpollClientLoginSlaveWrite.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o LinkToGameServerStaticVar.o ../gateway/LinkToGameServerStaticVar.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o LinkToGameServerProtocolParsing.o ../gateway/LinkToGameServerProtocolParsing.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o LinkToGameServer.o ../gateway/LinkToGameServer.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o TimerDdos.o ../gateway/TimerDdos.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o DatapackDownloaderBase.o ../gateway/DatapackDownloaderBase.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o DatapackDownloader_sub.o ../gateway/DatapackDownloader_sub.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o DatapackDownloader_main.o ../gateway/DatapackDownloader_main.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o DatapackDownloaderMainSub.o ../gateway/DatapackDownloaderMainSub.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollClientLoginSlaveDatapack.o ../gateway/EpollClientLoginSlaveDatapack.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o FacilityLibGateway.o ../gateway/FacilityLibGateway.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o main-epoll-gateway.o ../gateway/main-epoll-gateway.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o Epoll.o ../epoll/Epoll.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollGenericSslServer.o ../epoll/EpollGenericSslServer.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollGenericServer.o ../epoll/EpollGenericServer.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollClient.o ../epoll/EpollClient.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollSocket.o ../epoll/EpollSocket.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollSslClient.o ../epoll/EpollSslClient.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollClientToServer.o ../epoll/EpollClientToServer.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollSslClientToServer.o ../epoll/EpollSslClientToServer.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o EpollTimer.o ../epoll/EpollTimer.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o DatapackChecksum.o ../../client/libcatchchallenger/DatapackChecksum.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o TarDecode.o ../../client/libcatchchallenger/TarDecode.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o FacilityLibGeneral.o ../../general/base/FacilityLibGeneral.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o ProtocolParsingCheck.o ../../general/base/ProtocolParsingCheck.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o ProtocolParsingGeneral.o ../../general/base/ProtocolParsingGeneral.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o ProtocolParsingInput.o ../../general/base/ProtocolParsingInput.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o ProtocolParsingOutput.o ../../general/base/ProtocolParsingOutput.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o CommonSettingsCommon.o ../../general/base/CommonSettingsCommon.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o CommonSettingsServer.o ../../general/base/CommonSettingsServer.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o cpp11addition.o ../../general/base/cpp11addition.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o cpp11additionstringtointc.o ../../general/base/cpp11additionstringtointc.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o cpp11additionstringtointcpp.o ../../general/base/cpp11additionstringtointcpp.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o TinyXMLSettings.o ../base/TinyXMLSettings.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o tinyxml2.o ../../general/tinyXML2/tinyxml2.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o tinyxml2b.o ../../general/tinyXML2/tinyxml2b.cpp
clang++ -c -pipe -O2 -g -std=gnu++11 -D_REENTRANT -fPIC -DEPOLLCATCHCHALLENGERSERVER -DQT_NO_EMIT -DCATCHCHALLENGER_CLASS_GATEWAY -DCATCHCHALLENGER_XLMPARSER_TINYXML2 -DQT_QML_DEBUG -DQT_NO_DEBUG -I../gateway -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-clang -o tinyxml2c.o ../../general/tinyXML2/tinyxml2c.cpp
"""
content = content.strip().split('\n')

MAX_CONCURRENCE = multiprocessing.cpu_count()+1

length = len(content)
for i in range(MAX_CONCURRENCE):
    t = Requester()
    t.start()

main_thread = threading.currentThread()
for t in threading.enumerate():
    if t is not main_thread:
        t.join()

if not has_success:
	sys.exit(123)

os.system("clang++ -ccc-gcc-name g++ -o catchchallenger-gateway CompressionProtocol.o debug.o entropy_common.o error_private.o fse_decompress.o pool.o threading.o xxbhash.o zstd_common.o huf_decompress.o zstd_ddict.o zstd_decompress.o zstd_decompress_block.o fse_compress.o hist.o huf_compress.o zstd_compress.o zstd_compress_literals.o zstd_compress_sequences.o zstd_compress_superblock.o zstd_double_fast.o zstd_fast.o zstd_lazy.o zstd_ldm.o zstd_opt.o zstdmt_compress.o sha224.o EpollClientLoginSlave.o EpollServerLoginSlave.o EpollClientLoginSlaveStaticVar.o EpollClientLoginSlaveProtocolParsing.o EpollClientLoginSlaveWrite.o LinkToGameServerStaticVar.o LinkToGameServerProtocolParsing.o LinkToGameServer.o TimerDdos.o DatapackDownloaderBase.o DatapackDownloader_sub.o DatapackDownloader_main.o DatapackDownloaderMainSub.o EpollClientLoginSlaveDatapack.o FacilityLibGateway.o main-epoll-gateway.o Epoll.o EpollGenericSslServer.o EpollGenericServer.o EpollClient.o EpollSocket.o EpollSslClient.o EpollClientToServer.o EpollSslClientToServer.o EpollTimer.o DatapackChecksum.o TarDecode.o FacilityLibGeneral.o ProtocolParsingCheck.o ProtocolParsingGeneral.o ProtocolParsingInput.o ProtocolParsingOutput.o CommonSettingsCommon.o CommonSettingsServer.o cpp11addition.o cpp11additionstringtointc.o cpp11additionstringtointcpp.o TinyXMLSettings.o tinyxml2.o tinyxml2b.o tinyxml2c.o   -lcurl")
print('Finished')
if not has_success:
	sys.exit(123)
else:
	sys.exit(0)