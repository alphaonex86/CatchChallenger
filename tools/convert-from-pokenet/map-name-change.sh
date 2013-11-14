#!/bin/bash
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/Tileset /Tileset-/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/ 1.tsx/-1.tsx/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/ 2.tsx/-2.tsx/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/ 2.png/-2.png/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/ 1.png/-1.png/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/Kanto R/Kanto-R/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/Kanto T/Kanto-T/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/(d)/-d/g" {} \;
/usr/bin/nice -n 19 /usr/bin/ionice -c 3 /usr/bin/find ./ -type f -name '*.tsx' -or -name '*.tmx' -exec sed -i "s/.PNG/.png/g" {} \;
 
