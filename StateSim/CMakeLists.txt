add_library(uiiitstatesim SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/network.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/node.cpp
)

target_link_libraries(uiiitstatesim
  uiiitsupport
)
