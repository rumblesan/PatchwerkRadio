include_directories(include)
include_directories(../libpatchwerk/include)

set(TEST_SOURCES
    "src/test-audio-synthesis.c"
    "src/test-encoder.c"
    "src/test-filechunk.c"
    "src/test-messages.c"
    "src/test-patch-chooser.c"
    "src/test-config.c"
    "src/test-file-utils.c"
    )

foreach(TEST_FILE ${SOURCES} ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable("${TEST_NAME}" ${TEST_FILE})
    target_link_libraries("${TEST_NAME}" libpatchwerk)
    add_test("${TEST_NAME}" "${TEST_NAME}")
endforeach(TEST_FILE)
