# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem (no pointer resolution/recreate if restaure from cache like in datapack cache), copy with take care of use after free, less size (16Bits vs 32/64Bits)
* async the network to never wait the internet and timeout if apply
* NO general compression, to not try compress uncompresssible data and lower the CPU. Only compress repetitive data (and cache it if able).
* client side predicion and compute (like path finding) to scale better on server
* prefer not use template, hard to debug
* prefer not use auto, poorly detected into qt creator

# Server
* load datapack from disk or use cache, be able to external use cache generator to embed server with only load from cache (and drop heavy load from datapack)
* the map other player move have ACK, to drop send other update if not yet received (act as UDP)

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
* 253K stats

# More performance
* Async DB for server is better, actually only PostgreSQL
* LTO flag is better due to compreansive overall code, can optimize cross-file

# RAM needed
* objdump -h catchchallenger-server-cli-epoll | grep -A 1 ".bss"
* size catchchallenger-server-cli-epoll
* nm -S --size-sort -r catchchallenger-server-cli-epoll | head -n 5
* It needs RAM for the .data and .bss segments. Eg 10592 and 67118080
