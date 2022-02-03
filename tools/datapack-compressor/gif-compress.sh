#!/bin/bash
if [ -x /usr/bin/pngquant ]
then
	TEMPRANDOM=`< /dev/urandom /usr/bin/tr -dc A-Za-z0-9 | head -c16`
	for VARIABLE in `find ./ -name '*.gif' -and ! -name '* *' -type f`
	do
		echo "compress the file ${VARIABLE}"
		/usr/local/bin/gifsicle -O3 --lossy=80 -o /tmp/tmp${TEMPRANDOM}.png ${VARIABLE}
		RETURNCODE=$?
		if [ ${RETURNCODE} -ne 0 ]
		then
			echo "Gif creation failed ${RETURNCODE} for ${VARIABLE}"
        else
            mv /tmp/tmp${TEMPRANDOM}.png ${VARIABLE}
		fi
	done
fi

