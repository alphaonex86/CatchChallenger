#!/bin/bash
if [ -x /usr/bin/pngquant ]
then
	TEMPRANDOM=`< /dev/urandom tr -dc A-Za-z0-9 | head -c16`
	for VARIABLE in `find ./ -name '*.png' -and ! -name '* *' -type f`
	do
		echo "compress the file ${VARIABLE}"
		cat "${VARIABLE}" | /usr/bin/pngquant - --speed 1 > /tmp/tmp${TEMPRANDOM}.png
		mv /tmp/tmp${TEMPRANDOM}.png "${VARIABLE}"
	done
fi

