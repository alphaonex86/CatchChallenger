#!/bin/bash
if [ -x /usr/bin/convert ]
then
    TEMPRANDOM=`< /dev/urandom /usr/bin/tr -dc A-Za-z0-9 | head -c16`
    #-and ! -name '* *'
    find ./ -name overworld-shiny.png -type f -print0 | while read -d $'\0' VARIABLE
    do
        echo "convert t the file ${VARIABLE}"
        /usr/bin/convert ${VARIABLE} -fuzz '2%' -transparent '#ff00ff' /tmp/transparent.png
        mv /tmp/transparent.png ${VARIABLE}
    done
else
    echo "/usr/bin/convert not found"
fi
