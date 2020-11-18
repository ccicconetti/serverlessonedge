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
target_compile_options(
  hq
  PRIVATE
    -DFMT_LOCALE
    -DGFLAGS_IS_A_DLL=0
)
target_link_libraries(
    hq

-Wl,-rpath,/usr/local/lib:
    ${PROXYGENLIB}
    ${PROXYGENCURLLIB}
    ${PROXYGENHTTPSERVERLIB}
    libmvfst_transport.a
    libmvfst_client.a
    libmvfst_server.a
    ${PROXYGENLIB}
    libmvfst_server.a
    libwangle.a
/usr/lib/x86_64-linux-gnu/libzstd.so
/usr/lib/x86_64-linux-gnu/libboost_iostreams.so
/usr/lib/x86_64-linux-gnu/libboost_regex.so
    libmvfst_fizz_client.a
    libmvfst_client.a
    libmvfst_transport.a
    libmvfst_buf_accessor.a
    libmvfst_cc_algo.a
    libmvfst_codec_pktrebuilder.a
    libmvfst_looper.a
    libmvfst_happyeyeballs.a
    libmvfst_socketutil.a
    libmvfst_state_ack_handler.a
    libmvfst_loss.a
    libmvfst_state_simple_frame_functions.a
    libmvfst_state_qpr_functions.a
    libmvfst_d6d_state_functions.a
    libmvfst_state_functions.a
    libmvfst_codec_pktbuilder.a
    libmvfst_state_stream.a
    libmvfst_flowcontrol.a
    libmvfst_qlogger.a
    libmvfst_state_stream_functions.a
    libmvfst_state_pacing_functions.a
    libmvfst_fizz_handshake.a
    libmvfst_state_machine.a
    libmvfst_codec.a
    libmvfst_handshake.a
    libmvfst_codec_packet_number_cipher.a
    libmvfst_codec_decode.a
    libmvfst_d6d_types.a
    libmvfst_codec_types.a
    libmvfst_bufutil.a
    libmvfst_constants.a
    libmvfst_exception.a
    libfizz.a
    libfolly.a
/usr/lib/x86_64-linux-gnu/libevent.so
    libfmt.a
-pthread
/usr/local/lib/libz.so
/usr/lib/x86_64-linux-gnu/libbz2.so
/usr/lib/x86_64-linux-gnu/liblzma.so
/usr/lib/x86_64-linux-gnu/liblz4.so
/usr/lib/x86_64-linux-gnu/libzstd.so
/usr/lib/x86_64-linux-gnu/libsnappy.so
/usr/lib/x86_64-linux-gnu/libiberty.a
/usr/lib/x86_64-linux-gnu/libssl.so
/usr/lib/x86_64-linux-gnu/libcrypto.so
/usr/lib/x86_64-linux-gnu/libsodium.so
/usr/lib/x86_64-linux-gnu/libglog.so
/usr/lib/x86_64-linux-gnu/libgflags.so.2.2.1
/usr/lib/x86_64-linux-gnu/libdouble-conversion.so
-ldl
/usr/lib/x86_64-linux-gnu/librt.so
/usr/lib/x86_64-linux-gnu/libboost_context.so
/usr/lib/x86_64-linux-gnu/libboost_filesystem.so
/usr/lib/x86_64-linux-gnu/libboost_program_options.so
/usr/lib/x86_64-linux-gnu/libboost_regex.so
/usr/lib/x86_64-linux-gnu/libboost_system.so
/usr/lib/x86_64-linux-gnu/libboost_thread.so
-lpthread
/usr/lib/x86_64-linux-gnu/libboost_chrono.so
/usr/lib/x86_64-linux-gnu/libboost_date_time.so
/usr/lib/x86_64-linux-gnu/libboost_atomic.so
)

