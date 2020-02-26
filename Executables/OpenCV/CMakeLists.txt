add_library(uiiitdetectutils SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/detectutils.cpp
)

target_link_libraries(uiiitdetectutils
  uiiitcvutils
  uiiitedge
  uiiitsupport
  ${GLOG}
)

add_executable(edgecomputerfacerec
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputerfacerec.cpp
)

target_link_libraries(edgecomputerfacerec
  uiiitedge
  uiiitcvutils
  uiiitsupport
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(facedetect
  ${CMAKE_CURRENT_SOURCE_DIR}/facedetect.cpp
)

target_link_libraries(facedetect
  uiiitcvutils
  uiiitsupport
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(facerecclient
  ${CMAKE_CURRENT_SOURCE_DIR}/facerecclientmain.cpp
)

target_link_libraries(facerecclient
  uiiitcvutils
  uiiitedge
  uiiitdetectutils
  uiiitetsimec
  uiiitrest
  uiiitsupport
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(facerecpredict
  ${CMAKE_CURRENT_SOURCE_DIR}/facerecpredict.cpp
)

target_link_libraries(facerecpredict
  uiiitsupport
  ${GLOG}
  ${OpenCV_LIBS}
  ${Boost_LIBRARIES}
)

add_executable(facerectrain
  ${CMAKE_CURRENT_SOURCE_DIR}/facerectrain.cpp
)

target_link_libraries(facerectrain
  ${OpenCV_LIBS}
)
