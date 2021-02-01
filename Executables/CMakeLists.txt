if(WITH_OPENCV)
  add_subdirectory(OpenCV)
endif()

add_executable(clienttracegen
  ${CMAKE_CURRENT_SOURCE_DIR}/clienttracegenmain.cpp
)

target_link_libraries(clienttracegen
  uiiitsupport
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgecomputerclient
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputerclientmain.cpp
)

target_link_libraries(edgecomputerclient
  uiiitedge
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgeclient
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeclientmain.cpp
)

target_link_libraries(edgeclient
  uiiitsimulation
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgecontroller
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrollermain.cpp
)

target_link_libraries(edgecontroller
  uiiitedge
  uiiitetsimec
  ${GLOG}
  ${Boost_LIBRARIES}
)


add_executable(edgeippclient
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeippclientmain.cpp
)

target_link_libraries(edgeippclient
  uiiitsimulation
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgemulticlient
  ${CMAKE_CURRENT_SOURCE_DIR}/edgemulticlientmain.cpp
)

target_link_libraries(edgemulticlient
  uiiitedge
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(forwardingtableclient
  ${CMAKE_CURRENT_SOURCE_DIR}/forwardingtableclientmain.cpp
)

target_link_libraries(forwardingtableclient
  uiiitedge
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgecomputer
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputermain.cpp
)

target_link_libraries(edgecomputer
  uiiitedge
  ${LIBEDGEQUIC}
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgecomputerwsk
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputerwskmain.cpp
)

target_link_libraries(edgecomputerwsk
  uiiitedge
  ${LIBEDGEQUIC}
  uiiitopenwhisk
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgedispatcher
  ${CMAKE_CURRENT_SOURCE_DIR}/edgedispatchermain.cpp
)

target_link_libraries(edgedispatcher
  uiiitedge
  ${LIBEDGEQUIC}
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(edgerouter
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeroutermain.cpp
)

target_link_libraries(edgerouter
  uiiitedge
  ${LIBEDGEQUIC}
  ${GLOG}
  ${Boost_LIBRARIES}
)

add_executable(wskproxy
  ${CMAKE_CURRENT_SOURCE_DIR}/wskproxymain.cpp
)

target_link_libraries(wskproxy
  uiiitedge
  uiiitopenwhisk
  ${LIBEDGEQUIC}
  ${GLOG}
  ${Boost_LIBRARIES}
)
