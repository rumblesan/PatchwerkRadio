
include_directories(include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -D_POSIX_C_SOURCE=199309L")

set(SOURCES
  "src/audio_buffer.c"
  "src/audio_synthesis_process.c"
  "src/broadcast_process.c"
  "src/config.c"
  "src/encoder_process.c"
  "src/filechunk.c"
  "src/messages.c"
  "src/ogg_encoder.c"
  )

add_library(libpatchwerk ${SOURCES})

target_link_libraries(libpatchwerk bclib)
target_link_libraries(libpatchwerk shout m)
target_link_libraries(libpatchwerk config)
target_link_libraries(libpatchwerk vorbis vorbisfile ogg vorbisenc)
target_link_libraries(libpatchwerk pthread)
