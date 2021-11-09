if(NOT skip_google_test)

  # Download and unpack googletest at configure time
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Test/googletest-download )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Test/googletest-download )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                   ${CMAKE_BINARY_DIR}/googletest-build
                   EXCLUDE_FROM_ALL)

endif()

include_directories(${CMAKE_BINARY_DIR}/googletest-src/googletest/include)

set(QUICTESTS "")
if(WITH_QUIC)
  set(QUICTESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/testedgeserverquic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/testedgeclientmultiquic.cpp
  )
endif()

add_library(testedgelib SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/trivialedgecontrollerinstaller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testcallback.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testchain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testchaindagtransactiongrpc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testcomputer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testcomposer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testcontainer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testdag.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testedgeclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testedgeclientmulti.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testedgecontrolleretsi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testedgecontrolleretsibalanced.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testedgecontrollerflat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testedgecontrollerhier.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testedgemessages.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testetsitransaction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testforwardingtable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testlambda.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testlambdatransactiongrpc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testprocessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testptimeestimator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/teststatesim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/teststate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testtopology.cpp
  ${QUICTESTS}
)

target_link_libraries(testedgelib
  uiiitedge
  uiiitetsimec
  uiiitrest
  uiiitstatesim

  gtest
  ${Boost_LIBRARIES}
)

add_executable(testedge
  ${CMAKE_CURRENT_SOURCE_DIR}/testmain.cpp
)

target_link_libraries(testedge
  testedgelib
)

