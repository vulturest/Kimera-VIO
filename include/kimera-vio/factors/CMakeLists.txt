### Add source code for IDEs
target_sources(KimeraVIO PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/ParallelPlaneRegularFactor.h"
  "${CMAKE_CURRENT_LIST_DIR}/PointPlaneFactor.h"
)
target_include_directories(KimeraVIO PRIVATE ${CMAKE_CURRENT_LIST_DIR})

