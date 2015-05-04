#!/bin/sh
LOCKFILE=/tmp/check_interface.lock



# first skip
touch /tmp/check_interface.touch
if [ `find ${LOCKFILE} -newer /tmp/check_interface.touch | wc -l` -ne 0 ]
then
	echo 'interface detected as - file in the future'
	rm ${LOCKFILE}
fi
if [ ! -f ${LOCKFILE} ]
then
	echo 'interface detected as - (re)boot in progress'
	/usr/bin/touch ${LOCKFILE}
	exit;
fi
if [ `find ${LOCKFILE} -mmin -60 | wc -l` -ne 0 ]
then
	echo 'interface detected as - recent boot'
	exit;
fi




# soft detection
INTERNALONEISUP=0
for ip in 172.16.0.35 172.16.0.31
do
    /bin/ping -c 1 "$ip" > /dev/null
    if [ $? -eq 0 ]
    then
        INTERNALONEISUP=1
        break
    fi
done
EXTERNALONEISUP=0
for ip in 192.168.0.1 192.168.0.10
do
        /bin/ping -c 1 "$ip" > /dev/null
        if [ $? -eq 0 ]
        then
                EXTERNALONEISUP=1
                break
        fi
done
if [ ${EXTERNALONEISUP} -eq 0 ]
then
        echo 'interface external detected as down (soft restart)' >> /var/log/detected-as-interface-down-soft-restart.log 2>&1
        echo 'interface external detected as down (soft restart)'
        /etc/init.d/net.enp1s0 restart >> /var/log/detected-as-interface-down-soft-restart.log 2>&1
        exit;
fi
if [ ${INTERNALONEISUP} -eq 0 ]
then
        echo 'interface internal detected as down (soft restart)' >> /var/log/detected-as-interface-down-soft-restart.log 2>&1
        echo 'interface internal detected as down (soft restart)'
        /etc/init.d/net.eth0 restart >> /var/log/detected-as-interface-down-soft-restart.log 2>&1
        exit;
fi



# hard detection
INTERNALONEISUP=0
for ip in 172.16.0.35 172.16.0.31
do
	/bin/ping -c 1 "$ip" > /dev/null
	if [ $? -eq 0 ]
	then
		INTERNALONEISUP=1
		break
	fi
done
EXTERNALONEISUP=0
for ip in 192.168.0.1 192.168.0.10
do
        /bin/ping -c 1 "$ip" > /dev/null
        if [ $? -eq 0 ]
        then
                EXTERNALONEISUP=1
                break
        fi
done
if [ ${EXTERNALONEISUP} -eq 0 ]
then
        echo 'interface external detected as down'
fi
if [ ${INTERNALONEISUP} -eq 0 ]
then
        echo 'interface internal detected as down'
fi
if [ ${EXTERNALONEISUP} -ne 0 ] && [ ${INTERNALONEISUP} -ne 0 ]
then
        echo 'interface detected as up'
        exit;
fi




# actions
echo '' > /var/log/detected-as-interface-down.log
if [ ${INTERNALONEISUP} -ne 0 ]
then
        echo 'Internal interface is down' >> /var/log/detected-as-interface-down.log 2>&1
fi
if [ ${EXTERNALONEISUP} -ne 0 ]
then
        echo 'External interface is down' >> /var/log/detected-as-interface-down.log 2>&1
fi
/usr/sbin/ethtool eth0 >> /var/log/detected-as-interface-down.log 2>&1
/usr/sbin/ethtool enp1s0 >> /var/log/detected-as-interface-down.log 2>&1
/bin/ifconfig -a >> /var/log/detected-as-interface-down.log 2>&1
rm ${LOCKFILE}
echo 'interface detected as down'
exit;
reboot
