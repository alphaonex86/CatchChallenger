#!/bin/bash
if [ -x /usr/bin/pngquant ]
then
	TEMPRANDOM=`< /dev/urandom /usr/bin/tr -dc A-Za-z0-9 | head -c16`
	for VARIABLE in `find ./ -name '*.png' -and ! -name '* *'  -and ! -name '*.png.png' -type f`
	do
		echo "compress the file ${VARIABLE}"
		cat "${VARIABLE}" | /usr/bin/pngquant - --speed 1 > /tmp/tmp${TEMPRANDOM}.png
		RETURNCODE=$?
		if [ ${RETURNCODE} -ne 0 ]
		then
			echo "Png creation failed ${RETURNCODE} for ${VARIABLE}"
		else
            if [ -x /usr/bin/pngcrush ]
            then
                /usr/bin/pngcrush -rem gAMA -rem cHRM -rem iCCP -rem sRGB -force /tmp/tmp${TEMPRANDOM}.png "${VARIABLE}" > /dev/null 2>&1
            else
                mv /tmp/tmp${TEMPRANDOM}.png "${VARIABLE}"
            fi
		fi
	done
fi

