#!/bin/bash
if [ -x /usr/bin/php ]
then
	TEMPRANDOM=`< /dev/urandom tr -dc A-Za-z0-9 | head -c16`
	for VARIABLE in `find ./ -name '*.xml' -and ! -name '* *' -type f`
	do
		echo "compress the file ${VARIABLE}"
		/usr/bin/php "${BASH_SOURCE[0]}.php" "${VARIABLE}" > /tmp/file${TEMPRANDOM}
		mv /tmp/file${TEMPRANDOM} "${VARIABLE}"
	done
	for VARIABLE in `find ./ -name '*.tmx' -and ! -name '* *' -type f`
	do
		echo "compress the file ${VARIABLE}"
		/usr/bin/php "${BASH_SOURCE[0]}.php" "${VARIABLE}" > /tmp/file${TEMPRANDOM}
		mv /tmp/file${TEMPRANDOM} "${VARIABLE}"
	done
	for VARIABLE in `find ./ -name '*.tsx' -and ! -name '* *' -type f`
	do
		echo "compress the file ${VARIABLE}"
		/usr/bin/php "${BASH_SOURCE[0]}.php" "${VARIABLE}" > /tmp/file${TEMPRANDOM}
		mv /tmp/file${TEMPRANDOM} "${VARIABLE}"
	done
fi

