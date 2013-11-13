#!/bin/bash
if [ -x /usr/bin/pngquant ]
then
	TEMPRANDOM=`< /dev/urandom tr -dc A-Za-z0-9 | head -c16`
	for VARIABLE in `find ./ -name '*.png' -type f`
	do
		cat "${VARIABLE}" | /usr/bin/pngquant - --speed 1 > /tmp/tmp${TEMPRANDOM}.png
		mv /tmp/tmp${TEMPRANDOM}.png "${VARIABLE}"
	done
fi

