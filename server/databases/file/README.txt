== Why binary? ==
Improve performance
Less RAM

== Endian ==
Is endian server dependant, mean you can't create on big endian architecture and use on little endian architecture

== All in one version only ==
All is designed for little node only, mean it's only designed for all in one server

== Why not use database? ==
Due to cache/buffer, the data base need at least 100MB of RAM, then can't work with VM with 4/8MB of RAM
The database is mostly external process, generate at least 4x more cpu usage than internal only.

== Target ==
Target embedded system with <16MB RAM free, with very slow cpu usage, very few RAM
The target is anyone can test catchchallenger for free with their friends, then server with very very few player storage and online (you be able to fill a server with 10000+ server)
Can be integrated with busybox (use as standalone binary), on TV, modem, exotic architecture like MIPS BigEndian software float
Very very compact storage, no journal like normal DB (for the transactional part), simple binary file with very very high compression rate due to encoding

== Limits ==
To prevent abuse, technical problem, count limit:
  * the characters count is limited to 30 and hardcoded
  * the monsters count is limited and hardcoded
  * the online player is limited to 3 and hardcoded
No versioning system, if the server data base format change you need delete all the data
The format here IS GENERAL IDEA, the real format is defined by code, read the code to have the real format
Overwrite ALL the DB file each time, not just the changed part, eg: just move character and whole character file with monsters will be encoded and writed again
Characters will be writed only at disconnection, not safe if someone can crash the server. Server variable only is writen at server close (NOT KILL!)
