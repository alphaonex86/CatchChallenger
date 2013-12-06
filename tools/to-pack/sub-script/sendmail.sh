#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi
if [ "${CATCHCHALLENGER_VERSION}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

echo "Send mail..."
/usr/bin/php /home/first-world.info/catchchallenger/shop/sendmail_ultracopier_version.php ${CATCHCHALLENGER_VERSION}
echo "Send mail... done"
