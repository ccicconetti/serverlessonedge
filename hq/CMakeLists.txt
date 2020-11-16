# Copyright (c) Facebook, Inc. and its affiliates.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

set(PROXYGEN_ROOT "${PROJECT_SOURCE_DIR}/thirdparty/proxygen")
include_directories(SYSTEM ${PROXYGEN_ROOT})
include_directories(SYSTEM "${PROXYGEN_ROOT}/proxygen/_build/deps/include")
include_directories(SYSTEM "${PROXYGEN_ROOT}/proxygen/_build/generated")
set(PROXYGENLIB "${PROXYGEN_ROOT}/proxygen/_build/proxygen/lib/libproxygen.a")
set(PROXYGENCURLLIB "${PROXYGEN_ROOT}/proxygen/_build/proxygen/httpclient/samples/curl/libproxygencurl.a")
set(PROXYGENHTTPSERVERLIB "${PROXYGEN_ROOT}/proxygen/_build/proxygen/httpserver/libproxygenhttpserver.a")
link_directories("${PROXYGEN_ROOT}/proxygen/_build/deps/lib")
#proxygencurl
#proxygenhttpserver

add_executable(hq
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FizzContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HQClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HQLoggerHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HQServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HQParams.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleHandlers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PartiallyReliableCurlClient.cpp
)
target_link_libraries(
    hq


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

