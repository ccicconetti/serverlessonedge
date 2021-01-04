add_library(uiiitstatesim SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/affinity.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/element.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/job.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/link.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/node.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scenario.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/task.cpp
)

target_link_libraries(uiiitstatesim
  uiiitsupport
)
