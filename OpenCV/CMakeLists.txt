add_library(uiiitcvutils STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/cvutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/facedetectcomputer.cpp
)

target_link_libraries(uiiitcvutils
  uiiitedge
  uiiitsupport
  ${GLOG}
  ${OpenCV_LIBS}
)
