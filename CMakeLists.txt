cmake_minimum_required(VERSION 3.18)
project(edge_computing VERSION 0.1.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

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
find_package(CppREST REQUIRED) # populates CPP_REST_INCLUDE_DIR and CPP_REST_LIBRARY
if(WITH_OPENCV)
  MESSAGE("Compiling with OpenCV support")
  find_package(OpenCV REQUIRED) # http://opencv.org
endif()
if(WITH_QUIC)
  MESSAGE("Compiling with QUIC support")
endif()

set(PROXYGEN_ROOT "${PROJECT_SOURCE_DIR}/thirdparty/proxygen")

set(DISABLE_WARNINGS "-Wno-missing-field-initializers -Wno-unused-parameter -Wnon-virtual-dtor")
set(COMPILER_COMMON_FLAGS "-Wall -Wextra -Werror -DGTEST_CREATE_SHARED_LIBRARY=1 -DGTEST_LINKED_AS_SHARED_LIBRARY=1 -fPIC ${DISABLE_WARNINGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${COMPILER_COMMON_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${COMPILER_COMMON_FLAGS} -O2 -DNDEBUG")
if(WITH_QUIC)
  add_compile_definitions(WITH_QUIC)
endif()

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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# header of local libraries
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# local of header of external repos
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/etsimec)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/etsimec/rest)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/etsimec/rest/support)

# third-party includes from external repos
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/etsimec/third_party)
include_directories(thirdparty)

# external dependencies
set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

# optional library
set(LIBEDGEQUIC "")
if(WITH_QUIC)
  include_directories(SYSTEM "thirdparty/include")
  link_directories("thirdparty/libs")
  set(LIBEDGEQUIC uiiitedgequic)
endif()

# executables
add_subdirectory(Executables)

# local libraries
add_subdirectory(Edge)
add_subdirectory(LambdaMuSim)
add_subdirectory(Misc)
if(WITH_OPENCV)
  add_subdirectory(OpenCV)
endif()
if(WITH_QUIC)
  add_subdirectory(Quic)
endif()
add_subdirectory(Rpc)
add_subdirectory(Simulation)
add_subdirectory(StateSim)

# external repos
add_subdirectory(etsimec/Executables)
add_subdirectory(etsimec/EtsiMec)
add_subdirectory(etsimec/OpenWhisk)
add_subdirectory(etsimec/rest/Rest)
add_subdirectory(etsimec/rest/support/Dataset)
add_subdirectory(etsimec/rest/support/RpcSupport)
add_subdirectory(etsimec/rest/support/Support)
add_subdirectory(thirdparty/hungarian-algorithm-cpp)

# unit tests
string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
if (${CMAKE_BUILD_TYPE_LOWER} STREQUAL "debug")
  add_subdirectory(Test)
  set(skip_google_test TRUE)
  add_subdirectory(etsimec/Test)
  add_subdirectory(etsimec/rest/Test)
  add_subdirectory(etsimec/rest/support/Test)
endif()
