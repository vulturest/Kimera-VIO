### Add source code for stereoVIO
target_sources(KimeraVIO PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/Pipeline.h"
  "${CMAKE_CURRENT_LIST_DIR}/Pipeline-definitions.h"
)

target_include_directories(KimeraVIO PRIVATE ${CMAKE_CURRENT_LIST_DIR})

