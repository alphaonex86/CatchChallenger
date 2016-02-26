#!/bin/bash
if [ ! -x /usr/bin/lrelease ]
then
    echo "no /usr/bin/lrelease found!"
    exit;
fi
./1-pre-send.sh
./2-compil-wine32.sh
./4-clean-all.sh

