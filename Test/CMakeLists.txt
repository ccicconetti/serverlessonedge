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

include(GoogleTest)

add_library(testedgelib STATIC
  trivialedgecontrollerinstaller.cpp
)

add_dependencies(testedgelib uiiitrpc)

set(LIBS
  testedgelib
  uiiitedge
  uiiitetsimec
  uiiitlambdamusim
  uiiitrest
  uiiitstatesim
  hungarian
  gtest
  ${Boost_LIBRARIES}
)

add_executable(testcallback testmain.cpp testcallback.cpp)
target_link_libraries(testcallback ${LIBS})
gtest_discover_tests(testcallback)

add_executable(testchain testmain.cpp testchain.cpp)
target_link_libraries(testchain ${LIBS})
gtest_discover_tests(testchain)

add_executable(testchaindagtransactiongrpc testmain.cpp testchaindagtransactiongrpc.cpp)
target_link_libraries(testchaindagtransactiongrpc ${LIBS})
gtest_discover_tests(testchaindagtransactiongrpc)

add_executable(testcomputer testmain.cpp testcomputer.cpp)
target_link_libraries(testcomputer ${LIBS})
gtest_discover_tests(testcomputer)

add_executable(testcomposer testmain.cpp testcomposer.cpp)
target_link_libraries(testcomposer ${LIBS})
gtest_discover_tests(testcomposer)

add_executable(testcontainer testmain.cpp testcontainer.cpp)
target_link_libraries(testcontainer ${LIBS})
gtest_discover_tests(testcontainer)

add_executable(testdag testmain.cpp testdag.cpp)
target_link_libraries(testdag ${LIBS})
gtest_discover_tests(testdag)

add_executable(testedgeclient testmain.cpp testedgeclient.cpp)
target_link_libraries(testedgeclient ${LIBS})
gtest_discover_tests(testedgeclient)

add_executable(testedgeclientmulti testmain.cpp testedgeclientmulti.cpp)
target_link_libraries(testedgeclientmulti ${LIBS})
gtest_discover_tests(testedgeclientmulti)

add_executable(testedgecontrolleretsi testmain.cpp testedgecontrolleretsi.cpp)
target_link_libraries(testedgecontrolleretsi ${LIBS})
gtest_discover_tests(testedgecontrolleretsi)

add_executable(testedgecontrolleretsibalanced testmain.cpp testedgecontrolleretsibalanced.cpp)
target_link_libraries(testedgecontrolleretsibalanced ${LIBS})
gtest_discover_tests(testedgecontrolleretsibalanced)

add_executable(testedgecontrollerflat testmain.cpp testedgecontrollerflat.cpp)
target_link_libraries(testedgecontrollerflat ${LIBS})
gtest_discover_tests(testedgecontrollerflat)

add_executable(testedgecontrollerhier testmain.cpp testedgecontrollerhier.cpp)
target_link_libraries(testedgecontrollerhier ${LIBS})
gtest_discover_tests(testedgecontrollerhier)

add_executable(testedgemessages testmain.cpp testedgemessages.cpp)
target_link_libraries(testedgemessages ${LIBS})
gtest_discover_tests(testedgemessages)

add_executable(testetsitransaction testmain.cpp testetsitransaction.cpp)
target_link_libraries(testetsitransaction ${LIBS})
gtest_discover_tests(testetsitransaction)

add_executable(testforwardingtable testmain.cpp testforwardingtable.cpp)
target_link_libraries(testforwardingtable ${LIBS})
gtest_discover_tests(testforwardingtable)

add_executable(testhungarian testmain.cpp testhungarian.cpp)
target_link_libraries(testhungarian ${LIBS})
gtest_discover_tests(testhungarian)

add_executable(testlambda testmain.cpp testlambda.cpp)
target_link_libraries(testlambda ${LIBS})
gtest_discover_tests(testlambda)

add_executable(testlambdamusim testmain.cpp testlambdamusim.cpp)
target_link_libraries(testlambdamusim ${LIBS})
gtest_discover_tests(testlambdamusim)

add_executable(testlambdatransactiongrpc testmain.cpp testlambdatransactiongrpc.cpp)
target_link_libraries(testlambdatransactiongrpc ${LIBS})
gtest_discover_tests(testlambdatransactiongrpc)

add_executable(testmcfp testmain.cpp testmcfp.cpp)
target_link_libraries(testmcfp ${LIBS})
gtest_discover_tests(testmcfp)

add_executable(testprocessor testmain.cpp testprocessor.cpp)
target_link_libraries(testprocessor ${LIBS})
gtest_discover_tests(testprocessor)

add_executable(testptimeestimator testmain.cpp testptimeestimator.cpp)
target_link_libraries(testptimeestimator ${LIBS})
gtest_discover_tests(testptimeestimator)

add_executable(teststatesim testmain.cpp teststatesim.cpp)
target_link_libraries(teststatesim ${LIBS})
gtest_discover_tests(teststatesim)

add_executable(teststate testmain.cpp teststate.cpp)
target_link_libraries(teststate ${LIBS})
gtest_discover_tests(teststate)

add_executable(testtopology testmain.cpp testtopology.cpp)
target_link_libraries(testtopology ${LIBS})
gtest_discover_tests(testtopology)

if(WITH_QUIC)
  add_executable(testedgeserverquic testmain.cpp testedgeserverquic.cpp)
  target_link_libraries(testedgeserverquic uiiitedgequic ${LIBS})
  gtest_discover_tests(testedgeserverquic)
  
  add_executable(testedgeclientmultiquic testmain.cpp testedgeclientmultiquic.cpp)
  target_link_libraries(testedgeclientmultiquic uiiitedgequic ${LIBS})
  gtest_discover_tests(testedgeclientmultiquic)
endif()

if(WITH_MVFST)
  add_executable(testmvfst testmain.cpp testmvfst.cpp)
  target_link_libraries(testmvfst
    mvfst_codec_types
    mvfst_constants
    mvfst_exception
    fizz
    folly
    fmt
    iberty
    double-conversion
    ${LIBS}
  )
  gtest_discover_tests(testmvfst)
endif()