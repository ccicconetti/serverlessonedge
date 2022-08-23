set(PROTOS
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecomputer.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/edgecontroller.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/edgerouter.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/edgeserver.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/void.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

add_library(uiiitrpc STATIC
  ${PROTO_SRCS}
  ${GRPC_SRCS}
)

target_link_libraries(uiiitrpc
  ${GLOG}
  uiiitrpcsupport
  gRPC::grpc++_reflection
  protobuf::libprotobuf
)
