#!/bin/bash
if [ -x /usr/bin/php ]
then
	for VARIABLE in `find ./ -name '*.xml' -type f`
	do
		/usr/bin/php "${BASH_SOURCE[0]}.php" "${VARIABLE}" > /tmp/file
		mv /tmp/file "${VARIABLE}"
	done
	for VARIABLE in `find ./ -name '*.tmx' -type f`
	do
		/usr/bin/php "${BASH_SOURCE[0]}.php" "${VARIABLE}" > /tmp/file
		mv /tmp/file "${VARIABLE}"
	done
	for VARIABLE in `find ./ -name '*.tsx' -type f`
	do
		/usr/bin/php "${BASH_SOURCE[0]}.php" "${VARIABLE}" > /tmp/file
		mv /tmp/file "${VARIABLE}"
	done
fi

