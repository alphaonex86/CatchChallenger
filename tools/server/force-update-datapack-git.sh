#!/bin/bash
/etc/init.d/catchchallenger-server stop
echo 'download...'
cd /home/first-world.info/catchchallenger/datapack-git
/usr/bin/git pull
echo 'download... done'
echo 'rsync...'
rm -Rf /home/catchchallenger/datapack/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete /home/first-world.info/catchchallenger/datapack-git/datapack/ /home/catchchallenger/datapack/ --exclude=*.xcf --exclude='* *' --exclude=*.ogg --exclude=*/map.png > /dev/null 2>&1
echo 'rsync... done'
cd /home/catchchallenger/datapack/
echo 'clean...'
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /root/script/datapack-compressor/map-cleaner.sh > /dev/null 2>&1
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /root/script/datapack-compressor/png-compress.sh > /dev/null 2>&1
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /root/script/datapack-compressor/xml-compress.sh > /dev/null 2>&1
echo 'clean... done'
echo 'touch... '
/usr/bin/find /home/catchchallenger/datapack/ -type f -not \( -name "*.tmx" -or -name "*.xml" -or -name "*.tsx" -or -name "*.js" -or -name "*.png" -or -name "*.jpg" -or -name "*.gif" -or -name "*.ogg" -or -name "*.qml" -or -name "*.qm" -or -name "*.ts" -or -name "*.txt" \) -exec rm -f {} \;
cd /home/catchchallenger/datapack/
php /root/script/remove-wrong-datapack-file.php
find /home/catchchallenger/datapack/ -type d -empty -delete > /dev/null 2>&1
find /home/catchchallenger/datapack/ -type d -empty -delete > /dev/null 2>&1
find /home/catchchallenger/datapack/ -type d -empty -delete > /dev/null 2>&1
find /home/catchchallenger/datapack/ -type d -empty -delete > /dev/null 2>&1
find /home/catchchallenger/datapack/ -type d -empty -delete > /dev/null 2>&1
find /home/catchchallenger/datapack/ -exec touch -t 197001020000.00 {} \;
echo 'touch... done'
echo 'second rsync...'
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete /home/catchchallenger/datapack/ /home/first-world.info/catchchallenger/datapack-new/ > /dev/null 2>&1
mv /home/first-world.info/catchchallenger/datapack/ /home/first-world.info/catchchallenger/datapack-old/
mv /home/first-world.info/catchchallenger/datapack-new/ /home/first-world.info/catchchallenger/datapack/
rm -Rf /home/first-world.info/catchchallenger/datapack-old/
echo 'second rsync... done'
echo 'repack...'
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/git repack -a -d --depth=1 --window=1 > /dev/null 2>&1
echo 'repack... done'
cd /home/first-world.info/catchchallenger/
echo 'datapack list...'
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/php /root/script/datapack-list.php > /home/first-world.info/catchchallenger/datapack/datapack-list.txt
echo 'datapack list... done'
cd /home/catchchallenger/
mkdir -p /home/first-world.info/catchchallenger/datapack/pack/
echo 'datapack full tar.xz...'
tar --posix -c -f - datapack/ | xz -9 --check=crc32 > /home/first-world.info/catchchallenger/datapack/pack/datapack.tar.xz
echo 'datapack full tar.xz... done'
/etc/init.d/catchchallenger-server start
cd /home/first-world.info/catchchallenger/official-server/
echo 'datapack generator...'
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/php datapack-explorer-generator.php > /home/first-world.info/catchchallenger/datapack-explorer-generator.log 2>&1
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 chown lighttpd.lighttpd -Rf datapack-explorer-temp
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 mv datapack-explorer datapack-explorer-old
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 mv datapack-explorer-temp datapack-explorer
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 rm -Rf datapack-explorer-old
echo 'datapack generator... done'