#!/bin/bash
if [ -x /usr/bin/pngquant ]
then
	for VARIABLE in `find ./ -name '*.png' -type f`
	do
		cat "${VARIABLE}" | /usr/bin/pngquant - --speed 1 > /tmp/tmp.png
		mv /tmp/tmp.png "${VARIABLE}"
	done
fi

