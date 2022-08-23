add_library(uiiitsimulation STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ippclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/poissonclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/unifclient.cpp
)

target_link_libraries(uiiitsimulation
  uiiitedge
  uiiitrpc
  uiiitsupport
)
