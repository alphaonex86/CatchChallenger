#!/bin/bash
cd /root/generator/
if [ ! -f "index.php" ]
then
        echo "no file to generate the page"
        exit;
fi
/usr/bin/php index.php > /tmp/site-index.html.org
if [ ! -f "/tmp/site-index.html.org" ]
then
	echo "no file to update"
	exit;
fi
SERVERONLINE=`/bin/netstat -nap | /bin/grep -F LISTEN | /bin/grep -F tcp | /bin/grep -F catchchall | /bin/wc -l`
PLAYERONLINE=`/bin/netstat -nap | /bin/grep -F ESTABLISHED | /bin/grep -F tcp | /bin/grep -F catchchall | /bin/grep -F '127.0.0.1:42490' | /bin/wc -l`
echo "Have ${PLAYERONLINE} player online with ${SERVERONLINE} server online"
if [ ${SERVERONLINE} -eq 0 ]
then
        /bin/cat /tmp/site-index.html.org | /bin/sed "s/{player_online}/0/g" | /bin/sed "s/{server_stat}/<span style=\"color:red;\">down<\/span>/g" > /tmp/site-index.html
else
        /bin/cat /tmp/site-index.html.org | /bin/sed "s/{player_online}/${PLAYERONLINE}/g" | /bin/sed "s/{server_stat}/<span style=\"color:green;\">up<\/span>/g" > /tmp/site-index.html
fi
chown www-data.www-data /tmp/site-index.html
chmod 600 /tmp/site-index.html
cd /var/www
/usr/local/bin/zopfli --i1000 index.html
NEWDATE=19700102$(( ( RANDOM % 14 )  + 10 ))$(( ( RANDOM % 50 )  + 10 )).$(( ( RANDOM % 50 )  + 10 ))
chown www-data.www-data -Rf index.html.gz
touch -t ${NEWDATE} index.html
touch -t ${NEWDATE} index.html.gz

