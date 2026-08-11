# Point <DESTINATION> at <SOURCE> with a SYMLINK, so a build directory runs the
# very files of the sources: a template or a config edited in either place is the
# same file, and a file deleted from the sources disappears from the build
# directory too.
#
# It replaces a copy, which diverged in silence: five build directories of the map
# generators held five different settings.xml, one of them with a mapXCount of 64
# against the 4 of the checked-in file.
#
# A tool that writes its settings back does so THROUGH the link, so a linked
# config file MUST already hold every key that tool would add (maincode included).
# It then writes nothing and the file, comments included, stays byte identical
# (measured). The map generators only READ template/ skin/ tileset/, they write
# into dest/ (measured too).
#
# Windows without developer mode makes no symlink: there the file or folder is
# copied instead, which is the behaviour this replaces, so never worse.
#
# REPLACE=1 removes an existing real file/folder first. Use it ONLY where the
# build already threw that copy away on every build, never where someone may hold
# local edits: an existing settings.xml of a build directory is KEPT, with a
# message saying how to opt in.
if(IS_SYMLINK "${DESTINATION}")
    file(READ_SYMLINK "${DESTINATION}" CC_LINK_CURRENT)
    if(NOT CC_LINK_CURRENT STREQUAL "${SOURCE}")
        file(REMOVE "${DESTINATION}")
    endif()
elseif(EXISTS "${DESTINATION}")
    if(REPLACE)
        file(REMOVE_RECURSE "${DESTINATION}")
    else()
        message(STATUS "${DESTINATION} is a file of its own, kept. "
                       "Delete it to follow ${SOURCE} instead.")
    endif()
endif()
if(NOT IS_SYMLINK "${DESTINATION}" AND NOT EXISTS "${DESTINATION}")
    file(CREATE_LINK "${SOURCE}" "${DESTINATION}" RESULT CC_LINK_RESULT SYMBOLIC)
    if(NOT CC_LINK_RESULT EQUAL 0)
        if(IS_DIRECTORY "${SOURCE}")
            file(COPY "${SOURCE}/" DESTINATION "${DESTINATION}")
        else()
            configure_file("${SOURCE}" "${DESTINATION}" COPYONLY)
        endif()
        message(STATUS "${DESTINATION} copied, this platform makes no symlink: ${CC_LINK_RESULT}")
    endif()
endif()
