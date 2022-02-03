#!/bin/bash
rm -Rf /home/first-world.info/pkmn-i2p/*
cp /home/first-world.info/catchchallenger/pkmn-i2p-index.html /home/first-world.info/pkmn-i2p/index.html
/usr/bin/curl http://e2u26adqfhstjbg3avmz3fxffqq3oybix7utrxdkdo2jr4cjklfq/site.tar.xz --socks4a 127.0.0.1:15612 -o /home/first-world.info/pkmn-i2p/site.tar.xz
RETURNCODE=$?
if [ ${RETURNCODE} -ne 0 ]
then
        echo wget fail, ${RETURNCODE}
        exit
fi
cd /home/first-world.info/pkmn-i2p/
tar xJpf site.tar.xz > /dev/null
if [ ${RETURNCODE} -ne 0 ]
then
        echo wget fail, ${RETURNCODE}
        exit
fi
find ./ -exec touch -t 19700102$(( ( RANDOM % 14 )  + 10 ))$(( ( RANDOM % 50 )  + 10 )).$(( ( RANDOM % 50 )  + 10 )) "{}" \;
mkdir -p datapack/
cd datapack/
mkdir -p pack/
/usr/bin/curl http://e2u26adqfhstjbg3avmz3fxffqq3oybix7utrxdkdo2jr4cjklfq/datapack/pack/datapack.tar.xz --socks4a 127.0.0.1:15612 -o /home/first-world.info/pkmn-i2p/datapack/pack/datapack.tar.xz
RETURNCODE=$?
if [ ${RETURNCODE} -ne 0 ]
then
        echo wget fail, ${RETURNCODE}
        exit
fi
cd /home/first-world.info/pkmn-i2p/
tar xJpf /home/first-world.info/pkmn-i2p/datapack/pack/datapack.tar.xz > /dev/null
if [ ${RETURNCODE} -ne 0 ]
then
        echo wget fail, ${RETURNCODE}
        exit
fi
chown apache.apache -Rf /home/first-world.info/pkmn-i2p/
