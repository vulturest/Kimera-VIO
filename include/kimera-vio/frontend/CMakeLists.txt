### Add source code for IDEs
target_sources(KimeraVIO PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/CameraParams.h"
  "${CMAKE_CURRENT_LIST_DIR}/FeatureSelector.h"
  "${CMAKE_CURRENT_LIST_DIR}/Frame.h"
  "${CMAKE_CURRENT_LIST_DIR}/StereoFrame-definitions.h"
  "${CMAKE_CURRENT_LIST_DIR}/StereoFrame.h"
  "${CMAKE_CURRENT_LIST_DIR}/StereoImuSyncPacket.h"
  "${CMAKE_CURRENT_LIST_DIR}/StereoVisionFrontEnd-definitions.h"
  "${CMAKE_CURRENT_LIST_DIR}/StereoVisionFrontEnd.h"
  "${CMAKE_CURRENT_LIST_DIR}/Tracker-definitions.h"
  "${CMAKE_CURRENT_LIST_DIR}/Tracker.h"
  "${CMAKE_CURRENT_LIST_DIR}/VioFrontEndParams.h"
)
target_include_directories(KimeraVIO PRIVATE ${CMAKE_CURRENT_LIST_DIR})
