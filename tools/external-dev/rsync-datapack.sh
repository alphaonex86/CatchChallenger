#!/bin/bash
LOCALFULL=0
# put into game server
rm -Rf /usr/local/catchchallenger/datapack/
mkdir -p /usr/local/catchchallenger/datapack/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete /home/datapack/ /usr/local/catchchallenger/datapack/
# chmod
find /usr/local/catchchallenger/ -type d -exec chmod go=--- "{}" \;
find /usr/local/catchchallenger/ -type f -exec chmod go=--- "{}" \;
find /var/www/ -type d -exec chmod go=--- "{}" \;
find /var/www/ -type f -exec chmod go=--- "{}" \;
find /home/datapack/ -type d -exec chmod go=--- "{}" \;
find /home/datapack/ -type f -exec chmod go=--- "{}" \;
# touch it
cd /usr/local/catchchallenger/datapack/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /root/script/datapack-compressor/map-cleaner.sh > /dev/null 2>&1
#/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /root/script/datapack-compressor/png-compress.sh > /dev/null 2>&1
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /root/script/datapack-compressor/xml-compress.sh > /dev/null 2>&1
php /root/script/remove-wrong-datapack-file.php
#put into the site, light to do the site.tar.xz
rm -Rf /var/www/datapack/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete \
--exclude=*.xml --exclude=*.xcf --exclude=*.tmx --exclude=*.tsx --exclude=*.ogg --exclude=*.txt --exclude=*.xml --exclude=*.qml --exclude=*.qm  --exclude=*.ts --exclude=/map/ /usr/local/catchchallenger/datapack/ /var/www/datapack/
/usr/bin/find /var/www/datapack/ -type f -not \( -name "*.png" -or -name "*.jpg" -or -name "*.gif" \) -exec /usr/bin/ionice -c 3 /usr/bin/nice -n 19 rm -f "{}" \;
chown www-data.www-data -Rf /var/www/datapack/
find /var/www/datapack/ -type d -exec chmod go=--- "{}" \;
find /var/www/datapack/ -type f -exec chmod go=--- "{}" \;
find /var/www/datapack/ -exec touch -t 19700102$(( ( RANDOM % 14 )  + 10 ))$(( ( RANDOM % 50 )  + 10 )).$(( ( RANDOM % 50 )  + 10 )) "{}" \;
#chown the server
chown catchchallenger.catchchallenger -Rf /usr/local/catchchallenger/datapack/
#for mirror
rm -Rf /tmp/datapack/
mkdir -p /tmp/datapack/
/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -art --delete /usr/local/catchchallenger/datapack/ /tmp/datapack/ --exclude=*.xcf --exclude='* *' --exclude=*/map.png > /dev/null 2>&1
/usr/bin/find /tmp/datapack/ -type f -not \( -name "*.tmx" -or -name "*.xml" -or -name "*.tsx" -or -name "*.js" -or -name "*.png" -or -name "*.jpg" -or -name "*.gif" -or -name "*.ogg" -or -name "*.qml" -or -name "*.qm" -or -name "*.ts" -or -name "*.txt" \) -exec /usr/bin/ionice -c 3 /usr/bin/nice -n 19 rm -f "{}" \;
chown root.root -Rf /tmp/datapack/
cd /tmp/datapack/
find /tmp/datapack/ -type d -empty -delete > /dev/null 2>&1
find /tmp/datapack/ -type d -empty -delete > /dev/null 2>&1
find /tmp/datapack/ -type d -empty -delete > /dev/null 2>&1
find /tmp/datapack/ -type d -empty -delete > /dev/null 2>&1
find /tmp/datapack/ -type d -empty -delete > /dev/null 2>&1
find /tmp/datapack/ -exec touch -t 197001020000.00 "{}" \;
cd /tmp/
find datapack/ -type d -exec chmod go=--- "{}" \;
find datapack/ -type f -exec chmod go=--- "{}" \;
find datapack/ -type f -print | sort > /tmp/temporary_file_list
tar --owner=0 --group=0 --mtime='1970-01-01' --posix -c -f - -T /tmp/temporary_file_list | xz -9 --check=crc32 > /tmp/datapack.tar.xz
php /root/script/datapack-list.php > /tmp/datapack/datapack-list.txt
mkdir /tmp/datapack/pack/
mv /tmp/datapack.tar.xz /tmp/datapack/pack/
find /tmp/datapack/ -type d -exec chmod go=--- "{}" \;
find /tmp/datapack/ -type f -exec chmod go=--- "{}" \;
find /tmp/datapack/ -exec touch -t 197001020000.00 "{}" \;
cd /var/www/
find ./ -type f -print | sort | grep -F -v '.gz' | grep -v -F '.xz' | grep -v -F 'base.html' | grep -v -F 'robots.txt' | grep -v -F 'error/' | grep -v -F 'error.html' > /tmp/temporary_file_list
tar --owner=0 --group=0 --mtime='1970-01-01' --posix -c -f - -T /tmp/temporary_file_list | xz -9 --check=crc32 > /tmp/site.tar.xz
chmod go=--- /tmp/site.tar.xz
touch -t 19700102$(( ( RANDOM % 14 )  + 10 ))$(( ( RANDOM % 50 )  + 10 )).$(( ( RANDOM % 50 )  + 10 )) /tmp/site.tar.xz
if [ ${LOCALFULL} -ne 1 ]
then
	rm /var/www/site.tar.xz
	/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' --exclude='*.xz' /tmp/datapack/ root@192.168.0.1:/var/www/datapack/
	RETURNCODE=$?
	while [ ${RETURNCODE} -ne 0 ] && [ ${RETURNCODE} -ne 20 ]
	do
		/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' --exclude='*.xz' /tmp/datapack/ root@192.168.0.1:/var/www/datapack/
		RETURNCODE=$?
	done
	/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' /tmp/site.tar.xz root@192.168.0.1:/var/www/site.tar.xz
        RETURNCODE=$?
        while [ ${RETURNCODE} -ne 0 ] && [ ${RETURNCODE} -ne 20 ]
        do
                /usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' /tmp/site.tar.xz root@192.168.0.1:/var/www/site.tar.xz
                RETURNCODE=$?
        done
	rm -Rf /tmp/site.tar.xz
	/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' --exclude='*.xz' /var/www/datapack-explorer/ root@192.168.0.1:/var/www/datapack-explorer/
        RETURNCODE=$?
        while [ ${RETURNCODE} -ne 0 ] && [ ${RETURNCODE} -ne 20 ]
        do
                /usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' --exclude='*.xz' /var/www/datapack-explorer/ root@192.168.0.1:/var/www/datapack-explorer/
                RETURNCODE=$?
        done
	ssh root@192.168.0.1 'chown www-data.www-data -Rf /var/www/'
else
	mv /tmp/site.tar.xz /var/www/
	chown www-data.www-data -Rf /var/www/site.tar.xz
	/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -artc --delete /tmp/datapack/ /var/www/datapack/
        /usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' --exclude='*.xz' --exclude=/index.html --exclude='*base.html' --exclude='error.html' --exclude='/error/' /var/www/ root@192.168.0.1:/var/www/
        RETURNCODE=$?
        while [ ${RETURNCODE} -ne 0 ] && [ ${RETURNCODE} -ne 20 ]
        do
                /usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial --exclude='*.gz' --exclude='*.xz' --exclude=/index.html --exclude='*base.html' --exclude='error.html' --exclude='/error/' /var/www/ root@192.168.0.1:/var/www/
                RETURNCODE=$?
        done
	ssh root@192.168.0.1 'rm -Rf /var/www/index.html'
	/usr/bin/ionice -c 3 /usr/bin/nice -n 19 /usr/bin/rsync --rsync-path="/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/rsync" -e 'ssh -p 9899' -arvtcz --compress-level=9 --delete --progress --partial /tmp/site-index.html root@192.168.0.1:/var/www/index.html
    ssh root@192.168.0.1 'chown www-data.www-data -Rf /var/www/'
fi
rm -Rf /tmp/datapack/
