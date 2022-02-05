#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

cd ${CATCHCHALLENGERSOURCESPATH}/../CatchChallenger-datapack/
git pull
