#!/bin/bash
cd /home/first-world.info/catchchallenger/datapack-git
RCODE=`/usr/bin/git pull | grep -F 'Already up-to-date' | wc -l`
if [ ${RCODE} -ne 1 ]
then
	echo 'update needed'
	source force-update-datapack-git.sh
else
	echo 'update not needed'
fi
