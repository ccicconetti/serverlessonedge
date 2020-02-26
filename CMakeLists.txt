cmake_policy(SET CMP0048 NEW)

project(edge_computing VERSION 0.1.6)
cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 14) # c++14

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
find_package(OpenCV REQUIRED) # http://opencv.org
find_package(CppREST REQUIRED) # populates CPP_REST_INCLUDE_DIR and CPP_REST_LIBRARY

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/etsimec/third_party)

# external dependencies
set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

# executables
add_subdirectory(Executables)

# local libraries
add_subdirectory(Edge)
add_subdirectory(OpenCV)
add_subdirectory(Rpc)
add_subdirectory(Simulation)

# external repos
add_subdirectory(etsimec/Executables)
add_subdirectory(etsimec/EtsiMec)
add_subdirectory(etsimec/OpenWhisk)
add_subdirectory(etsimec/rest/Rest)
add_subdirectory(etsimec/rest/support/RpcSupport)
add_subdirectory(etsimec/rest/support/Support)

# unit tests
if (${CMAKE_BUILD_TYPE} STREQUAL "debug")
  add_subdirectory(Test)
endif()
