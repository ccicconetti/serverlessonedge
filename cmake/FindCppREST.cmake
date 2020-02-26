IF(UNIX)
    FIND_PATH(CPP_REST_INCLUDE_DIR http_client.h
      /usr/include
      /usr/include/cpprest
      /usr/local/include
      /usr/local/include/cpprest
      "$ENV{LIB_DIR}/include"
      "$ENV{LIB_DIR}/include/cpprest"
      "${CMAKE_SOURCE_DIR}/include"
      "${CMAKE_SOURCE_DIR}/include/cpprest"
      #mingw
      c:/msys/local/include
      NO_DEFAULT_PATH
      )

    SET(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a" ".lib" ".dylib")
    FIND_LIBRARY(CPP_REST_LIBRARY NAMES cpprest PATHS
      $ENV{LIB}
      /usr/lib
      /usr/lib/x86_64-linux-gnu/
      /usr/local/lib
      "$ENV{LIB_DIR}/lib"
      "${CMAKE_SOURCE_DIR}/lib"
      #mingw
      c:/msys/local/lib
      NO_DEFAULT_PATH
      )
ELSE()
    FIND_PATH(CPP_REST_INCLUDE_DIR http_client.h
        "${PROJECT_ROOT}/include/cpprest"
      )

    FILE(GLOB CPP_REST_LIBRARY NAMES
        "${PROJECT_ROOT}/lib/cpprest*.lib"
        "${PROJECT_ROOT}/lib/cpprest*.a"
        )
ENDIF()


IF(CPP_REST_INCLUDE_DIR AND CPP_REST_LIBRARY)
  MESSAGE(STATUS "Found C++ REST")
ELSE()
  MESSAGE(FATAL_ERROR "Could not find C++ REST")
ENDIF()
