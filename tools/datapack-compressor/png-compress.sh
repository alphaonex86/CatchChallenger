#!/bin/bash
minimumsize=64
if [ -x /usr/bin/pngquant ]
then
    TEMPRANDOM=`< /dev/urandom /usr/bin/tr -dc A-Za-z0-9 | head -c16`
    #-and ! -name '* *'
    find ./ -name '*.png' -and ! -name '*.png.png' -type f -print0 | while read -d $'\0' VARIABLE
    do
        actualsize=$(wc -c <"${VARIABLE}")
        if [ $actualsize -le $minimumsize ]
        then
            echo size is under $minimumsize bytes something wrong file "${VARIABLE}" step 1
            tail /tmp/png-compress.log
            exit
        fi
        echo "compress the file ${VARIABLE}"
        cat "${VARIABLE}" | /usr/bin/pngquant - --speed 1 > /tmp/tmp${TEMPRANDOM}.png
        RETURNCODE=$?
        if [ ${RETURNCODE} -ne 0 ]
        then
            echo "/usr/bin/pngquant Png creation failed ${RETURNCODE} for ${VARIABLE}"
        else
            actualsize=$(wc -c <"/tmp/tmp${TEMPRANDOM}.png")
            if [ $actualsize -ge $minimumsize ]
            then
                if [ -x /usr/bin/pngcrush ]
                then
                    /usr/bin/pngcrush -rem gAMA -rem cHRM -rem iCCP -rem sRGB -force /tmp/tmp${TEMPRANDOM}.png /tmp/tmp${TEMPRANDOM}2.png > /tmp/png-compress.log 2>&1
                    if [ ${RETURNCODE} -ne 0 ]
                    then
                        echo "/usr/bin/pngcrush Png creation failed ${RETURNCODE} for ${VARIABLE}"
                    fi
                else
                    echo "/usr/bin/pngcrush not found"
                    mv /tmp/tmp${TEMPRANDOM}.png /tmp/tmp${TEMPRANDOM}2.png
                fi
                actualsize=$(wc -c <"/tmp/tmp${TEMPRANDOM}2.png")
                if [ $actualsize -le $minimumsize ]
                then
                    echo size is under $minimumsize bytes something wrong file /tmp/tmp${TEMPRANDOM}2.png
                    tail /tmp/png-compress.log
                    exit
                fi
                ZOPFLI="/usr/bin/zopflipng"
                if [ -x ${ZOPFLI} ]
                then
                    rm -f "${VARIABLE}"
                    #! \warn /usr/bin/zopflipng -> disabled because froze at idle, 0% of cpu, do nothing, all blocked, temporal solution: remove the file before do it
                    #${ZOPFLI} --iterations=50 --splitting=3 --filters=01234mepb --lossy_8bit --lossy_transparent /tmp/tmp${TEMPRANDOM}2.png "${VARIABLE}" > /tmp/png-compress.log 2>&1
                    ${ZOPFLI} --iterations=500 --lossy_8bit --lossy_transparent /tmp/tmp${TEMPRANDOM}2.png "${VARIABLE}" > /tmp/png-compress.log 2>&1
                    # not work for big image size, ehoeks-zopfli-png give: uncompress returned Z_BUF_ERROR
                    #${ZOPFLI} --iterations=50 -c --png /tmp/tmp${TEMPRANDOM}2.png > "${VARIABLE}" 2> /tmp/png-compress.log
                    RETVAR=$?
                    if [ ${RETVAR} -ne 0 ]
                    then
                        echo zopfli have failed
                        cat /tmp/png-compress.log
                        cp /tmp/tmp${TEMPRANDOM}2.png "${VARIABLE}"
                    else
                        if [ -f "${VARIABLE}" ]
                        then
                            actualsize=$(wc -c <"${VARIABLE}")
                            if [ $actualsize -lt $minimumsize ]
                            then
                                echo size is under $minimumsize bytes zopfli seem have failed
                                mv /tmp/tmp${TEMPRANDOM}2.png "${VARIABLE}"
                            fi
                        else
                            echo zopfli have failed or not file
                            cat /tmp/png-compress.log
                            cp /tmp/tmp${TEMPRANDOM}2.png "${VARIABLE}"
                        fi
                    fi
                else
                    mv /tmp/tmp${TEMPRANDOM}2.png "${VARIABLE}"
                fi
            else
                echo size is under $minimumsize bytes /usr/bin/pngquant seem have failed
            fi
        fi
        if [ -f "${VARIABLE}" ]
        then
            actualsize=$(wc -c <"${VARIABLE}")
            if [ $actualsize -le $minimumsize ]
            then
                echo size is under $minimumsize bytes something wrong file ${VARIABLE} step 2
                tail /tmp/png-compress.log
                exit
            fi
        else
            echo error ${VARIABLE} is not a file
        fi
    done
else
    echo "/usr/bin/pngquant not found"
fi

rm /tmp/png-compress.log
