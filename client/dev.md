# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem, copy with take care of use after free (path finding), less size (16Bits vs 32/64Bits)
* async the ressources loading to never freeze the interface
* async the network to never wait the internet and timeout if apply
* client side predicion and compute (like path finding) to scale better on server

# arguments, mostly to debug
* --server name: on screen selection, auto select the server with this name
* --autologin: on login/pass page, automaticlly try login
* --character name: on character selection page, automaticly select character with this name
* --closewhenonmap: close 1s after the character is spawn on map
* --dropsenddataafteronmap: dropany output comunicacion from client to server after the player spawn on map, used to explore datapack map, ignore zone monster colision (to not open fight with wild monster) and any fight
