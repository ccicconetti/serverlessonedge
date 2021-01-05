add_library(uiiitsimulation SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ippclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/poissonclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/unifclient.cpp
)

target_link_libraries(uiiitsimulation
  uiiitedge
  uiiitedgefactory
  uiiitrpc
  uiiitsupport
)
