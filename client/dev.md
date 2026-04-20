# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem, copy with take care of use after free (path finding), less size (16Bits vs 32/64Bits)
* async the ressources loading to never freeze the interface
* async the network to never wait the internet and timeout if apply
* client side predicion and compute (like path finding) to scale better on server

# arguments, mostly to debug
* --server name: on screen selection, auto select the server with this name
* --host/--name: alternative to --server, not add entry to server list
* --autosolo: enter automatically to the solo game
* --autologin: on login/pass page, automaticlly try login
* --character name: on character selection page, automaticly select character with this name
* --closewhenonmap: close 1s after the character is spawn on map
* --closewhenonmapafter=10: similar --closewhenonmap but when on map then change direction each 1s (if look botton, look top, if look left, look right, ...) and close after the number of second specified
* --dropsenddataafteronmap: dropany output comunicacion from client to server after the player spawn on map, used to explore datapack map, ignore zone monster colision (to not open fight with wild monster) and any fight
