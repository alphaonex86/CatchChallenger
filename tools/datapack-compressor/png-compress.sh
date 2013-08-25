#!/bin/bash
for VARIABLE in `find ./ -name '*.png'`
do
	cat ${VARIABLE} | pngquant - --speed 1 > /tmp/tmp.png
	mv /tmp/tmp.png ${VARIABLE}
done
