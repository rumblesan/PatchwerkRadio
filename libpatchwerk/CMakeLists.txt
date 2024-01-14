
include_directories(include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -D_POSIX_C_SOURCE=199309L")

set(SOURCE_HEADERS
  "include/api_process.h"
  "include/audio_buffer.h"
  "include/audio_synthesis_process.h"
  "include/broadcast_process.h"
  "include/config.h"
  "include/encoder_process.h"
  "include/file_utils.h"
  "include/filechunk.h"
  "include/logging.h"
  "include/messages.h"
  "include/ogg_encoder.h"
  "include/patch_chooser_process.h"
  "include/pipe_utils.h"
  )

set(SOURCES
  "src/api_process.c"
  "src/audio_buffer.c"
  "src/audio_synthesis_process.c"
  "src/broadcast_process.c"
  "src/config.c"
  "src/encoder_process.c"
  "src/filechunk.c"
  "src/messages.c"
  "src/ogg_encoder.c"
  "src/pipe_utils.c"
  "src/file_utils.c"
  "src/patch_chooser_process.c"
  )

add_library(libpatchwerk ${SOURCES} ${SOURCES_HEADERS})

target_link_libraries(libpatchwerk bclib)
target_link_libraries(libpatchwerk ck)
target_link_libraries(libpatchwerk shout m)
target_link_libraries(libpatchwerk config)
target_link_libraries(libpatchwerk vorbis vorbisfile ogg vorbisenc)
target_link_libraries(libpatchwerk pthread)
target_link_libraries(libpatchwerk pd)
target_link_libraries(libpatchwerk lua5.4)
