#!/bin/bash
for VARIABLE in `find ./ -name '*.xml' -type f`
do
	php "${BASH_SOURCE[0]}.php" "${VARIABLE}" > /tmp/file
	mv /tmp/file "${VARIABLE}"
done
