#!/bin/bash
for VARIABLE in `find ./ -name '*.xml'`
do
	php ${BASH_SOURCE[0]}.php ${VARIABLE} > /tmp/file
	mv /tmp/file ${VARIABLE}
done
