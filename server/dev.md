# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem (no pointer resolution/recreate if restaure from cache like in datapack cache), copy with take care of use after free, less size (16Bits vs 32/64Bits)
* async the network to never wait the internet and timeout if apply
* load datapack from disk or use cache, be able to external use cache generator to embed server with only load from cache (and drop heavy load from datapack)
* the map other player move have ACK, to drop send other update if not yet received (act as UDP)
* NO general compression, to not try compress uncompresssible data and lower the CPU. Only compress repetitive data (and cache it if able).
* client side predicion and compute (like path finding) to scale better on server
* in view of low end hardware, with 1 or low thread count, it's better to scale doing: 1 CPU for catchchallenger, 1 CPU of database, ... or scale catchchallenger via cluster using 1 CPU by game server. use multithread do more complex code, do 4x power consumption if quad core and only few % more performance. Can have performance improvament on GPU but requiere perfect multithread via messages queue and good map management multithread.
