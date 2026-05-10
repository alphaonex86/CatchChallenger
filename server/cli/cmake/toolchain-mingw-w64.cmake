# toolchain-mingw-w64.cmake — cross-compile catchchallenger-server-cli
# from Linux to Windows x86_64 with the mingw-w64 GCC tools.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Gentoo's mingw layout (`dev-util/mingw64-toolchain`) lives entirely
# under /usr/lib/mingw64-toolchain. Fallback to the Debian-style
# /usr/x86_64-w64-mingw32 path if the Gentoo one isn't there.
if(EXISTS /usr/lib/mingw64-toolchain/x86_64-w64-mingw32)
    set(CMAKE_FIND_ROOT_PATH /usr/lib/mingw64-toolchain/x86_64-w64-mingw32)
elseif(EXISTS /usr/x86_64-w64-mingw32)
    set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
