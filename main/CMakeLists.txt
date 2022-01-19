
include_directories(include)

set(SOURCES "src/main.c")

add_executable(patchwerk ${SOURCES})

target_link_libraries(patchwerk libpatchwerk)

install(TARGETS patchwerk EXPORT PatchwerkRadioTargets
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
