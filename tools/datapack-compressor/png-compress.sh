#!/bin/bash
if [ -x /usr/bin/pngquant ]
then
	TEMPRANDOM=`< /dev/urandom /usr/bin/tr -dc A-Za-z0-9 | head -c16`
	for VARIABLE in `find ./ -name '*.png' -and ! -name '* *' -type f`
	do
		echo "compress the file ${VARIABLE}"
		cat "${VARIABLE}" | /usr/bin/pngquant - --speed 1 > /tmp/tmp${TEMPRANDOM}.png
		RETURNCODE=$?
		if [ ${RETURNCODE} -ne 0 ]
		then
			echo "Png creation failed ${RETURNCODE} for ${VARIABLE}"
		else
			mv /tmp/tmp${TEMPRANDOM}.png "${VARIABLE}"
		fi
	done
fi

