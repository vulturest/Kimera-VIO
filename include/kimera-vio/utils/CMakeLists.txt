### Add includes for stereoVIO
target_sources(KimeraVIO
    PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Accumulator.h"
    "${CMAKE_CURRENT_LIST_DIR}/Histogram.h"
    "${CMAKE_CURRENT_LIST_DIR}/Statistics.h"
    "${CMAKE_CURRENT_LIST_DIR}/ThreadsafeImuBuffer.h"
    "${CMAKE_CURRENT_LIST_DIR}/ThreadsafeImuBuffer-inl.h"
    "${CMAKE_CURRENT_LIST_DIR}/ThreadsafeQueue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ThreadsafeTemporalBuffer.h"
    "${CMAKE_CURRENT_LIST_DIR}/ThreadsafeTemporalBuffer-inl.h"
    "${CMAKE_CURRENT_LIST_DIR}/Timer.h"
    "${CMAKE_CURRENT_LIST_DIR}/UtilsGeometry.h"
    "${CMAKE_CURRENT_LIST_DIR}/UtilsOpenCV.h"
    "${CMAKE_CURRENT_LIST_DIR}/YamlParser.h"
)
target_include_directories(KimeraVIO PRIVATE ${CMAKE_CURRENT_LIST_DIR})

