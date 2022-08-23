add_library(uiiitlambdamusim STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/appperiods.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/apppool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mcfp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scenario.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/simulation.cpp
)

target_link_libraries(uiiitlambdamusim
  uiiitdataset
  uiiitstatesim
  uiiitsupport
)

add_executable(lambdamusim
  ${CMAKE_CURRENT_SOURCE_DIR}/lambdamusimmain.cpp
)

target_link_libraries(lambdamusim
  uiiitlambdamusim

  hungarian

  ${GLOG}
  ${Boost_LIBRARIES}
)
