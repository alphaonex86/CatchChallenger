# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem, copy with take care of use after free (path finding), less size (16Bits vs 32/64Bits)
* async the ressources loading to never freeze the interface
* async the network to never wait the internet and timeout if apply
* client side predicion and compute (like path finding) to scale better on server

# arguments, mostly to debug
* Server selection — pick **exactly one** of the following three options:
  * --server NAME: select an existing server entry from the server list (loaded from the internet feed or saved by the user previously). The entry can be either TCP or WebSocket.
  * --host HOST + --port PORT: TCP direct-connect using ad-hoc credentials. NOT added to the server list. Both required together. Available only when CATCHCHALLENGER_NO_TCPSOCKET is NOT defined.
  * --url URL: WebSocket direct-connect (`ws://...` or `wss://...`). NOT added to the server list. Available only when CATCHCHALLENGER_NO_WEBSOCKET is NOT defined.
  Combining any two (e.g. --server + --url) is rejected by the parser; pass exactly one.
* --autosolo: enter automatically to the solo game
* --autologin: on login/pass page, automaticlly try login
* --character name: on character selection page, automaticly select character with this name
* --closewhenonmap: close 1s after the character is spawn on map
* --closewhenonmapafter=10: similar --closewhenonmap but when on map then change direction each 1s (if look botton, look top, if look left, look right, ...) and close after the number of second specified
* --dropsenddataafteronmap: dropany output comunicacion from client to server after the player spawn on map, used to explore datapack map, ignore zone monster colision (to not open fight with wild monster) and any fight
* --take-screenshot=PATH: (qtopengl) once on the map, write the rendered viewport to PATH and exit; pins srand(42) so random tile variants and animation offsets are reproducible (used by testingmap4client.py)
* --main-datapack-code=CODE: override the autosolo maincode under datapack/internal/map/main/, instead of picking the first directory alphabetically (used by testingmap4client.py to force the "test" fixture)
