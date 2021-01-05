add_library(uiiitedgefactory SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/edgeclientfactory.cpp
)

target_link_libraries(uiiitedgefactory
  uiiitedge
  uiiitedgequic
  ${GLOG}
)