#!/bin/bash
if [ ! -f "/home/first-world.info/catchchallenger/official-server-org.html" ]
then
        echo "no file to update"
        exit;
fi
SERVERONLINE=`/bin/netstat -nap | /bin/grep -F LISTEN | /bin/grep -F tcp | /bin/grep -F catchchall | /bin/wc -l`
PLAYERONLINE=`/bin/netstat -nap | /bin/grep -F ESTABLISHED | /bin/grep -F tcp | /bin/grep -F catchchall | /bin/grep -F '37.59.242.80:42489' | /bin/wc -l`
echo "Have ${PLAYERONLINE} player online with ${SERVERONLINE} server online"
if [ ${SERVERONLINE} -eq 0 ]
then
	/bin/cat /home/first-world.info/catchchallenger/official-server-org.html | /bin/sed "s/{player_online}/0/g" | /bin/sed "s/{server_stat}/<span style=\"color:red;\">down<\/span>/g" > /home/first-world.info/catchchallenger/official-server.html
else
	/bin/cat /home/first-world.info/catchchallenger/official-server-org.html | /bin/sed "s/{player_online}/${PLAYERONLINE}/g" | /bin/sed "s/{server_stat}/<span style=\"color:green;\">up<\/span>/g" > /home/first-world.info/catchchallenger/official-server.html
fi
