# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem (no pointer resolution/recreate if restaure from cache like in datapack cache), copy with take care of use after free, less size (16Bits vs 32/64Bits)
* async the network to never wait the internet and timeout if apply
* NO general compression, to not try compress uncompresssible data and lower the CPU. Only compress repetitive data (and cache it if able).
* client side predicion and compute (like path finding) to scale better on server
* prefer not use template, hard to debug
* prefer not use auto, poorly detected into qt creator
* prefer while over for

# Server
* load datapack from disk or use cache, be able to external use cache generator to embed server with only load from cache (and drop heavy load from datapack)
* the map other player move have ACK, to drop send other update if not yet received (act as UDP)

# Client
* All all ressources to drop load time and don't have hugly loading like hole or widget position changes during loading

## Client arguments, mostly to debug
* --server name: on screen selection, auto select the server with this name
* --autosolo: auto login into solo
* --autologin: on login/pass page, automaticlly try login
* --character name: on character selection page, automaticly select character with this name
* --closewhenonmap: close 1s after the character is spawn on map
* --dropsenddataafteronmap: dropany output comunicacion from client to server after the player spawn on map, used to explore datapack map, ignore zone monster colision (to not open fight with wild monster) and any fight
### Alternative to server name
* --host hostorip: host or ip to connect (need specify the port too)
* --port number: port number to connect (need specify the host too)

# Naming Conventions
* Classes: PascalCase (e.g., EpollClientLoginMaster)
* Methods/Functions: camelCase
* snake_case with a trailing underscore for member variables
* Enum values: PascalCase

# Code Organization
* Headers: #ifndef/#define/#endif guards
* Namespace: CatchChallenger
* Includes: Relative paths with ../../ (e.g., #include "../../general/base/FacilityLibGeneral.hpp")

# Code Style
* Indentation: 4 spaces
* Braces: K&R style (same line)
* No comments unless essential
* Initializer lists: Used in constructors
* STL containers: std::unordered_set, std::vector, std::string

# Key Patterns
* Static variable declaration: ClassName *ClassName::variable=NULL; (in .cpp)
* Enum defined inline: enum Name : std::uint8_t { };
* Preprocessor for optional params in constructor headers

# Output console Style
binary size for std::cout and std::cerr: with debug symbol
* 5092K stats
*  991K stats-upx
std::print with debug symbol
* 6399K stats
* 1245K stats
release with
QMAKE_CXXFLAGS += -Os -flto -fno-exceptions
QMAKE_CFLAGS += -Os -flto -fno-exceptions
QMAKE_LFLAGS += -Os -flto -s
* 253K stats release llvm std::cout
* 345K  stats release llvm std::print
