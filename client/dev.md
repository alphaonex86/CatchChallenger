# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem, copy with take care of use after free (path finding), less size (16Bits vs 32/64Bits)
* async the ressources loading to never freeze the interface
* async the network to never wait the internet and timeout if apply
* client side predicion and compute (like path finding) to scale better on server
