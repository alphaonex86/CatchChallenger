#!/bin/bash
CATCHCHALLENGERPLATFORM='test'
CFLAGS='-O2 -march=native'
CATCHCHALLENGERDETAILS="${CFLAGS}"
GITFOLDER='/home/user/Desktop/CatchChallenger/git/'
CACHEFOLDER='/mnt/perso/other/catchchallengerbenchmark/'
URLTOSEND='http://amber/benchmark-tracking/send.php'
URLTOSENDKEY='eYGhNVwlKq2ErXlL'
QTQMAKE='/usr/local/Qt-5.2.1/bin/qmake'
SERVERPROPERTIES="/home/user/Desktop/CatchChallenger/build-catchchallenger-server-cli-epoll-Qt5_5_2-Debug/server.properties"
CATCHCHALLENGERDATAPACK="/home/user/Desktop/CatchChallenger/build-catchchallenger-server-cli-epoll-Qt5_5_2-Debug/datapack/"
TMPFOLDER="/tmp/benchmarkcatchchallenger/"

cd ${GITFOLDER}
killall -s 9 yes > /dev/null 2>&1
CPUCOUNT=`grep -c ^processor /proc/cpuinfo`

#compil the tools
/usr/bin/rsync -art --delete ${GITFOLDER} ${TMPFOLDER}
/usr/bin/rsync -art --delete ${CATCHCHALLENGERDATAPACK} /tmp/datapack/
cd ${TMPFOLDER}/tools/benchmark-cli/
${QTQMAKE}
make -j${CPUCOUNT} >> /dev/null 2>&1
mv benchmark-cli /tmp/benchmark-cli
if [ ! -x /tmp/benchmark-cli ]
then
    echo "benchmark-cli application not found in /tmp/benchmark-cli"
    exit;
fi

for (( c=1; c<=${CPUCOUNT}; c++ ))
do
   nice -n 19 yes > /dev/null 2>&1 &
done

git pull --rebase
COMMITLIST=`git log --reverse --pretty=format:"%H" --date=short | tail -n 15`
for COMMIT in ${COMMITLIST}
do
    mkdir -p ${CACHEFOLDER}
    CACHEFILE=${CACHEFOLDER}/${COMMIT}.txt
    if [ ! -e ${CACHEFILE} ]
    then
        /usr/bin/rsync -art --delete ${GITFOLDER} ${TMPFOLDER}
        cd ${TMPFOLDER}
        git reset --hard ${COMMIT}
        if [ -e ${TMPFOLDER}/server/ ] && [ -e ${TMPFOLDER}/tools/benchmark-cli/repport.sh ]
        then
            echo 'Init'
            sed -i 's/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/CATCHCHALLENGER_EXTRA_CHECK/g' general/base/GeneralVariable.h
            sed -i 's/#define CATCHCHALLENGER_SERVER_EXTRA_CHECK/\/\/CATCHCHALLENGER_SERVER_EXTRA_CHECK/g' server/VariableServer.h
            echo '' >> server/catchchallenger-server-cli-epoll.pro
            echo 'DEFINES += SERVERBENCHMARK' >> server/catchchallenger-server-cli-epoll.pro
            echo "QMAKE_CFLAGS=\"${CFLAGS}\"" >> server/catchchallenger-server-cli-epoll.pro
            echo "QMAKE_CXXFLAGS=\"${CFLAGS}\"" >> server/catchchallenger-server-cli-epoll.pro
            echo '' > ${CACHEFILE}

            cd ${TMPFOLDER}/server/
            ${QTQMAKE} catchchallenger-server-cli-epoll.pro
            make -j${CPUCOUNT} > ${TMPFOLDER}/fail.log 2>&1
            echo 'Make catchchallenger-server-cli-epoll done'
            if [ -x catchchallenger-server-cli-epoll ]
            then
                echo 'catchchallenger-server-cli-epoll rsync'
                /usr/bin/rsync -art --delete ${SERVERPROPERTIES} ${TMPFOLDER}/server/
                /usr/bin/rsync -art --delete ${CATCHCHALLENGERDATAPACK} ${TMPFOLDER}/server/datapack/
                if [ -e ${TMPFOLDER}/tools/benchmark-cli/ ] && [ -e ${TMPFOLDER}/tools/benchmark-cli/repport.sh ]
                then
                    echo 'benchmark-cli start'
                    /tmp/benchmark-cli > ${CACHEFILE} 2>&1
                    echo 'benchmark-cli done'
                    cd ${TMPFOLDER}

                    BENCHMARKVALUE=`cat ${CACHEFILE} | grep 'Result internal to connect all player' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    BENCHMARKVALUEIDLE=`cat ${CACHEFILE} | grep 'Result internal to idle server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    BENCHMARKVALUEMOVE=`cat ${CACHEFILE} | grep 'Result internal to moving on server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    BENCHMARKVALUECHAT=`cat ${CACHEFILE} | grep 'Result internal to chat' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    BENCHMARKVALUEEXTERNAL=`cat ${CACHEFILE} | grep 'Result to connect all player' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    BENCHMARKVALUEIDLEEXTERNAL=`cat ${CACHEFILE} | grep 'Result to idle server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    BENCHMARKVALUEMOVEEXTERNAL=`cat ${CACHEFILE} | grep 'Result to moving on server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    BENCHMARKVALUECHATEXTERNAL=`cat ${CACHEFILE} | grep 'Result to chat' | sed -r 's/.* ([0-9]+).*/\1/g'`
                    if [ `cat ${CACHEFILE} | grep 'Result internal to connect all player' | wc -l` -ne 1 ]
                    then
                        echo "bug: application not found for commit ${COMMIT}"
                        exit;
                    fi
                    echo '' > ${CACHEFOLDER}/${COMMIT}-ok.txt
                    echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
                    /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
                    echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS} external&connectAllPlayer=${BENCHMARKVALUEEXTERNAL}&idle=${BENCHMARKVALUEIDLEEXTERNAL}&move=${BENCHMARKVALUEMOVEEXTERNAL}&chat=${BENCHMARKVALUECHATEXTERNAL}"
                    /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS} external&connectAllPlayer=${BENCHMARKVALUEEXTERNAL}&idle=${BENCHMARKVALUEIDLEEXTERNAL}&move=${BENCHMARKVALUEMOVEEXTERNAL}&chat=${BENCHMARKVALUECHATEXTERNAL}"
                else
                    echo '' > ${CACHEFOLDER}/${COMMIT}-skip.txt
                fi
            else
                echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
                /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
            fi
        else
            echo '' > ${CACHEFOLDER}/${COMMIT}-skip.txt
        fi
    else
        if [ ! -e ${CACHEFOLDER}/${COMMIT}-skip.txt ]
        then
            if [ -e ${CACHEFOLDER}/${COMMIT}-ok.txt ]
            then
                BENCHMARKVALUE=`cat ${CACHEFILE} | grep 'Result internal to connect all player' | sed -r 's/.* ([0-9]+).*/\1/g'`
                BENCHMARKVALUEIDLE=`cat ${CACHEFILE} | grep 'Result internal to idle server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                BENCHMARKVALUEMOVE=`cat ${CACHEFILE} | grep 'Result internal to moving on server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                BENCHMARKVALUECHAT=`cat ${CACHEFILE} | grep 'Result internal to chat' | sed -r 's/.* ([0-9]+).*/\1/g'`
                BENCHMARKVALUEEXTERNAL=`cat ${CACHEFILE} | grep 'Result to connect all player' | sed -r 's/.* ([0-9]+).*/\1/g'`
                BENCHMARKVALUEIDLEEXTERNAL=`cat ${CACHEFILE} | grep 'Result to idle server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                BENCHMARKVALUEMOVEEXTERNAL=`cat ${CACHEFILE} | grep 'Result to moving on server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                BENCHMARKVALUECHATEXTERNAL=`cat ${CACHEFILE} | grep 'Result to chat' | sed -r 's/.* ([0-9]+).*/\1/g'`
                if [ `cat ${CACHEFILE} | grep 'Result internal to connect all player' | wc -l` -eq 1 ]
                then
                    echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
                    /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
                    echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS} external&connectAllPlayer=${BENCHMARKVALUEEXTERNAL}&idle=${BENCHMARKVALUEIDLEEXTERNAL}&move=${BENCHMARKVALUEMOVEEXTERNAL}&chat=${BENCHMARKVALUECHATEXTERNAL}"
                    /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS} external&connectAllPlayer=${BENCHMARKVALUEEXTERNAL}&idle=${BENCHMARKVALUEIDLEEXTERNAL}&move=${BENCHMARKVALUEMOVEEXTERNAL}&chat=${BENCHMARKVALUECHATEXTERNAL}"
                fi
            else
                echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
                /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
            fi
        fi
    fi
done
rm -Rf ${TMPFOLDER}
killall -s 9 yes > /dev/null 2>&1
