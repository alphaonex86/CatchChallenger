#!/bin/bash
CATCHCHALLENGERPLATFORM='test'
CATCHCHALLENGERDETAILS='-O2 -march=native'
GITFOLDER='/home/user/Desktop/CatchChallenger/git/'
CACHEFOLDER='/mnt/perso/other/catchchallengerbenchmark/'
URLTOSEND='http://amber/benchmark-tracking/send.php'
URLTOSENDKEY='eYGhNVwlKq2ErXlL'
QTQMAKE='/usr/local/Qt-5.2.1/bin/qmake'
SERVERPROPERTIES="/home/user/Desktop/CatchChallenger/build-catchchallenger-server-cli-epoll-Qt5_5_2-Debug/server.properties"
CATCHCHALLENGERDATAPACK="/home/user/Desktop/CatchChallenger/build-catchchallenger-server-cli-epoll-Qt5_5_2-Debug/datapack/"
TMPFOLDER="/tmp/benchmarkcatchchallenger/"
cd ${GITFOLDER}
git pull --rebase
COMMITLIST=`git log --reverse --pretty=format:"%H" --date=short | tail -n 5`
for COMMIT in ${COMMITLIST}
do
    CACHEFILE=${CACHEFOLDER}/${COMMIT}.txt
    if [ ! -e ${CACHEFILE} ]
    then
        /usr/bin/rsync -art --delete ${GITFOLDER} ${TMPFOLDER}
        cd ${TMPFOLDER}
        git reset --hard ${COMMIT}
        if [ -e ${TMPFOLDER}/server/ ]
        then
            sed -i 's/#define CATCHCHALLENGER_EXTRA_CHECK/\/\/CATCHCHALLENGER_EXTRA_CHECK/g' general/base/GeneralVariable.h
            sed -i 's/#define CATCHCHALLENGER_SERVER_EXTRA_CHECK/\/\/CATCHCHALLENGER_SERVER_EXTRA_CHECK/g' server/VariableServer.h
            echo '' >> server/catchchallenger-server-cli-epoll.pro
            echo 'DEFINES += SERVERBENCHMARK' >> server/catchchallenger-server-cli-epoll.pro
            echo '' > ${CACHEFILE}

            cd ${TMPFOLDER}/server/
            ${QTQMAKE} catchchallenger-server-cli-epoll.pro
            make -j4 > ${TMPFOLDER}/fail.log 2>&1
            if [ -x catchchallenger-server-cli-epoll ]
            then
                /usr/bin/rsync -art --delete ${SERVERPROPERTIES} ${TMPFOLDER}/server/datapack/
                /usr/bin/rsync -art --delete ${CATCHCHALLENGERDATAPACK} ${TMPFOLDER}/server/datapack/
                if [ -e ${TMPFOLDER}/tools/benchmark-cli/ ] && [ -e ${TMPFOLDER}/tools/benchmark-cli/repport.sh ]
                then
                    cd ${TMPFOLDER}/tools/benchmark-cli/
                    ${QTQMAKE}
                    make -j4 > ${TMPFOLDER}/fail.log 2>&1
                    ./benchmark-cli > ${CACHEFILE} 2>&1
                    if [ -x benchmark-cli ]
                    then
                        cd ${TMPFOLDER}
                        echo '' > ${CACHEFOLDER}/${COMMIT}-ok.txt

                        BENCHMARKVALUE=`cat ${CACHEFILE} | grep 'Result internal to connect all player' | sed -r 's/.* ([0-9]+).*/\1/g'`
                        BENCHMARKVALUEIDLE=`cat ${CACHEFILE} | grep 'Result internal to idle server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                        BENCHMARKVALUEMOVE=`cat ${CACHEFILE} | grep 'Result internal to moving on server' | sed -r 's/.* ([0-9]+).*/\1/g'`
                        BENCHMARKVALUECHAT=`cat ${CACHEFILE} | grep 'Result internal to chat' | sed -r 's/.* ([0-9]+).*/\1/g'`
                        echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
                        /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
                    else
                        echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
                        /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
                    fi
                else
                    echo '' > ${CACHEFOLDER}/${COMMIT}-skip.txt
                fi
            else
                echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
                /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
            fi
        else
            echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
            /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
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
                echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
                /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&connectAllPlayer=${BENCHMARKVALUE}&idle=${BENCHMARKVALUEIDLE}&move=${BENCHMARKVALUEMOVE}&chat=${BENCHMARKVALUECHAT}"
            else
                echo /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
                /usr/bin/wget -O - -q "${URLTOSEND}?commit=${COMMIT}&key=${URLTOSENDKEY}&platform=${CATCHCHALLENGERPLATFORM}&details=${CATCHCHALLENGERDETAILS}&failed"
            fi
        fi
    fi
done
rm -Rf ${TMPFOLDER}