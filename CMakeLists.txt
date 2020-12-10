cmake_policy(SET CMP0048 NEW)

project(edge_computing VERSION 0.1.6)
cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 17)

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(APPLE)
  set(OPENSSL_LIBRARIES /usr/local/opt/openssl/lib/libssl.dylib /usr/local/opt/openssl/lib/libcrypto.dylib)
  set(OPENSSL_INCLUDE_DIR /usr/local/opt/openssl/include)
  find_package(glog REQUIRED)
  set(GLOG glog::glog)
else()
  find_package(OpenSSL REQUIRED)
  set(GLOG glog)
endif()
find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
set(Boost_USE_MULTITHREADED TRUE)
find_package(Boost
  REQUIRED
  COMPONENTS program_options filesystem system chrono thread
)
if(WITH_OPENCV)
  find_package(OpenCV REQUIRED) # http://opencv.org
endif()
find_package(CppREST REQUIRED) # populates CPP_REST_INCLUDE_DIR and CPP_REST_LIBRARY

set(PROXYGEN_ROOT "${PROJECT_SOURCE_DIR}/thirdparty/proxygen")

set (DISABLE_WARNINGS "-Wno-missing-field-initializers -Wno-unused-parameter -Wnon-virtual-dtor")
set (COMPILER_COMMON_FLAGS "-Wall -Wextra -Werror -DGTEST_CREATE_SHARED_LIBRARY=1 -DGTEST_LINKED_AS_SHARED_LIBRARY=1 -fPIC ${DISABLE_WARNINGS}")
set (CMAKE_CXX_FLAGS_DEBUG "${COMPILER_COMMON_FLAGS} -g -O0")
set (CMAKE_CXX_FLAGS_RELEASE "${COMPILER_COMMON_FLAGS} -O2 -DNDEBUG")

MESSAGE("============CONFIGURATION SUMMARY================")
MESSAGE("")
MESSAGE("CMAKE_SOURCE_DIR:         ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE("COMPILER:                 ${CMAKE_CXX_COMPILER}")
MESSAGE("COMPILER FLAGS DEBUG:     ${CMAKE_CXX_FLAGS_DEBUG}")
MESSAGE("COMPILER FLAGS RELEASE:   ${CMAKE_CXX_FLAGS_RELEASE}")
MESSAGE("CMAKE_BUILD_TYPE:         ${CMAKE_BUILD_TYPE}")
MESSAGE("Protobuf Library:         ${PROTOBUF_LIBRARY}")
MESSAGE("Protobuf Includes Path:   ${PROTOBUF_INCLUDE_DIR}")
MESSAGE("CppREST Library:          ${CPP_REST_LIBRARY}")
MESSAGE("CppREST Includes Path:    ${CPP_REST_INCLUDE_DIR}")
MESSAGE("OpenSSL Libraries:        ${OPENSSL_LIBRARIES}")
MESSAGE("OpenSSL Includes Path:    ${OPENSSL_INCLUDE_DIR}")

# header of local libraries
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# local of header of external repos
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/etsimec)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/etsimec/rest)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/etsimec/rest/support)

# third-party includes from external repos
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/etsimec/third_party)

# external dependencies
set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(SYSTEM ${PROXYGEN_ROOT})
include_directories(SYSTEM "${PROXYGEN_ROOT}/proxygen/_build/deps/include")
include_directories(SYSTEM "${PROXYGEN_ROOT}/proxygen/_build/generated")
link_directories("${PROXYGEN_ROOT}/proxygen/_build/deps/lib")

# executables
add_subdirectory(Executables)

# local libraries
add_subdirectory(Edge)
if(WITH_OPENCV)
  add_subdirectory(OpenCV)
endif()
add_subdirectory(Quic)
add_subdirectory(Rpc)
add_subdirectory(Simulation)

#add_subdirectory(hq)

# external repos
add_subdirectory(etsimec/Executables)
add_subdirectory(etsimec/EtsiMec)
add_subdirectory(etsimec/OpenWhisk)
add_subdirectory(etsimec/rest/Rest)
add_subdirectory(etsimec/rest/support/RpcSupport)
add_subdirectory(etsimec/rest/support/Support)

# unit tests
string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
if (${CMAKE_BUILD_TYPE_LOWER} STREQUAL "debug")
  add_subdirectory(Test)
  set(skip_google_test TRUE)
  add_subdirectory(etsimec/Test)
  add_subdirectory(etsimec/rest/Test)
  add_subdirectory(etsimec/rest/support/Test)
endif()
