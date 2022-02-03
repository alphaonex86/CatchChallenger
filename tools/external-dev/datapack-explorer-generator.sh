#!/bin/bash
cd /root/generator
rm -Rf /var/www/datapack-explorer-temp/
mkdir -p /var/www/datapack-explorer-temp/
php datapack-explorer-generator.php
cd /var/www/
chown www-data.www-data -Rf datapack-explorer-temp
mv datapack-explorer datapack-explorer-old
mv datapack-explorer-temp datapack-explorer
rm -Rf datapack-explorer-old
chmod -Rf go=--- datapack-explorer/
find datapack-explorer/ -name '*.gz' -exec rm -Rf {} \;
echo compression
find datapack-explorer/ -print -name '*.html' -exec /usr/bin/ionice -c 3 /usr/bin/nice -n 19 zopfli --i1000 {} \;
find datapack-explorer/ -exec touch -t 19700102$(( ( RANDOM % 14 )  + 10 ))$(( ( RANDOM % 50 )  + 10 )).$(( ( RANDOM % 50 )  + 10 )) "{}" \;
touch -t 19700102$(( ( RANDOM % 14 )  + 10 ))$(( ( RANDOM % 50 )  + 10 )).$(( ( RANDOM % 50 )  + 10 )) datapack-explorer/
