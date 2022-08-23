add_library(uiiitstatesim STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/affinity.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/element.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/job.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/link.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/node.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scenario.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/simulation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/task.cpp
)

target_link_libraries(uiiitstatesim
  uiiitsupport
)

add_executable(statesim
  ${CMAKE_CURRENT_SOURCE_DIR}/statesimmain.cpp
)

target_link_libraries(statesim
  uiiitstatesim

  ${GLOG}
  ${Boost_LIBRARIES}
)
