#!/bin/bash

cd ../../
find ./ -name "Thumbs.db" -exec rm {} \; >> /dev/null 2>&1
find ./ -name ".directory" -exec rm {} \; >> /dev/null 2>&1

echo "Send sources..."
/usr/bin/rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --delete --partial --progress /home/user/Desktop/CatchChallenger/client/ root@ssh.first-world.info:/root/catchchallenger/sources/client/ --exclude='*build*' --exclude='*Qt_5*' --exclude='*qt5*' --exclude='*.pro.user' --exclude='*.qm'
/usr/bin/rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --delete --partial --progress /home/user/Desktop/CatchChallenger/general/ root@ssh.first-world.info:/root/catchchallenger/sources/general/ --exclude='*build*' --exclude='*Qt_5*' --exclude='*qt5*' --exclude='*.pro.user' --exclude='*.qm'
/usr/bin/rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --delete --partial --progress /home/user/Desktop/CatchChallenger/server/ root@ssh.first-world.info:/root/catchchallenger/sources/server/ --exclude='*build*' --exclude='*Qt_5*' --exclude='*qt5*' --exclude='*.pro.user' --exclude='*.qm'
/usr/bin/rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --delete --partial --progress /home/user/Desktop/CatchChallenger/tools/ root@ssh.first-world.info:/root/catchchallenger/sources/tools/ --exclude='*build*' --exclude='*Qt_5*' --exclude='*qt5*' --exclude='*.pro.user' --exclude='*.qm' --exclude=/to-pack/
echo "Send sources... done"



