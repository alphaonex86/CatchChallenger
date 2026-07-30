# Copy SOURCE to DESTINATION only when DESTINATION does not exist yet: the
# settings file of a build directory belongs to that build (it is edited there).
if(NOT EXISTS "${DESTINATION}")
    configure_file("${SOURCE}" "${DESTINATION}" COPYONLY)
endif()
