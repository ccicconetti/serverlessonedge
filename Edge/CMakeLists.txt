include_directories(SYSTEM ${PROXYGEN_ROOT})
include_directories(SYSTEM "${PROXYGEN_ROOT}/proxygen/_build/deps/include")
include_directories(SYSTEM "${PROXYGEN_ROOT}/proxygen/_build/generated")
set(PROXYGENLIB "${PROXYGEN_ROOT}/proxygen/_build/proxygen/lib/libproxygen.a")
set(PROXYGENCURLLIB "${PROXYGEN_ROOT}/proxygen/_build/proxygen/httpclient/samples/curl/libproxygencurl.a")
set(PROXYGENHTTPSERVERLIB "${PROXYGEN_ROOT}/proxygen/_build/proxygen/httpserver/libproxygenhttpserver.a")
link_directories("${PROXYGEN_ROOT}/proxygen/_build/deps/lib")

add_library(uiiitedge SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/Detail/printtable.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/Entries/entry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Entries/entryleastimpedance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Entries/entryroundrobin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Entries/entryrandom.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Entries/entryproportionalfairness.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/composer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/container.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeclientfactory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeclientmulti.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeclientpool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputerclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputerserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputerwsk.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontroller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrollerclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrolleretsi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrolleretsibalanced.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrollerflat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrollerhier.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrollermessages.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontrollerserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgedispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgelambdaprocessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgelambdaprocessoroptions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgemessages.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgerouter.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/edgeserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeserverimpl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeservergrpc.cpp
  
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeserveroptions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/etsiedgeclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/forwardingtable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/forwardingtableclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/forwardingtablefactory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/forwardingtableinterface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/forwardingtableserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lambda.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localoptimizer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localoptimizerasync.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localoptimizerasyncpf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localoptimizerfactory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localoptimizerperiodic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localoptimizertrivial.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/processloadserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/processor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/processortype.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ptimeestimator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ptimeestimatordelay.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ptimeestimatorfactory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ptimeestimatorprobe.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ptimeestimatorrtt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ptimeestimatorutil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rttestimator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/topology.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utilestimator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wskproxy.cpp
)

target_link_libraries(uiiitedge
  uiiitetsimec
  uiiitopenwhisk
  uiiitrpc
  uiiitsupport
)

add_library(uiiitedgequic STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeserverquic.cpp
)

target_link_libraries(uiiitedgequic
    ${PROXYGENLIB}
    ${PROXYGENCURLLIB}
    ${PROXYGENHTTPSERVERLIB}
    mvfst_transport
    mvfst_client
    mvfst_server
    ${PROXYGENLIB}
    mvfst_server
    wangle
    zstd
    boost_iostreams
    boost_regex
    mvfst_fizz_client
    mvfst_client
    mvfst_transport
    mvfst_buf_accessor
    mvfst_cc_algo
    mvfst_codec_pktrebuilder
    mvfst_looper
    mvfst_happyeyeballs
    mvfst_socketutil
    mvfst_state_ack_handler
    mvfst_loss
    mvfst_state_simple_frame_functions
    mvfst_state_qpr_functions
    mvfst_d6d_state_functions
    mvfst_state_functions
    mvfst_codec_pktbuilder
    mvfst_state_stream
    mvfst_flowcontrol
    mvfst_qlogger
    mvfst_state_stream_functions
    mvfst_state_pacing_functions
    mvfst_fizz_handshake
    mvfst_state_machine
    mvfst_codec
    mvfst_handshake
    mvfst_codec_packet_number_cipher
    mvfst_codec_decode
    mvfst_d6d_types
    mvfst_codec_types
    mvfst_bufutil
    mvfst_constants
    mvfst_exception
    fizz
    folly
    event
    fmt

    pthread
    z
    bz2
    lzma
    lz4
    zstd
    snappy
    iberty.a
    ssl
    crypto
    sodium
    glog
    gflags
    double-conversion
    dl
    rt
    boost_context
    boost_filesystem
    boost_program_options
    boost_regex
    boost_system
    boost_thread
    pthread
    boost_chrono
    boost_date_time
    boost_atomic
)
