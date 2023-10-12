set(PROTOS
  sdifi/speech/v1alpha/speech.proto
  sdifi/events/v1alpha/event.proto
  google/rpc/status.proto)

list(TRANSFORM PROTOS PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")

add_custom_command(
  OUTPUT ${PROTOS}
  #TODO(rkjaran): make this point to the git repo
  COMMAND buf export https://github.com/sdifi/protos.git -o ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running Buf export for SDiFI"
  VERBATIM)


set(TSC_PROTOS
  google/api/annotations.proto
  google/api/client.proto
  google/api/field_behavior.proto
  google/api/http.proto
  tiro/speech/v1alpha/speech.proto
)

list(TRANSFORM TSC_PROTOS PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")

add_custom_command(
  OUTPUT ${TSC_PROTOS}
  #TODO(rkjaran): make this point to the git repo
  COMMAND buf export https://github.com/tiro-is/tiro-speech-core.git --path=proto --exclude-imports -o ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND cmake -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/proto/tiro ${CMAKE_CURRENT_BINARY_DIR}/tiro
  COMMAND cmake -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/proto/google ${CMAKE_CURRENT_BINARY_DIR}/google
  COMMENT "Running Buf export for Tiro Speech Core"
  VERBATIM)

set(PROTOS "${PROTOS};${TSC_PROTOS}")

set(PROTO_HDRS "${PROTOS}")
set(PROTO_SRCS "${PROTOS}")
list(TRANSFORM PROTO_HDRS REPLACE "\\.proto" ".pb.h")
list(TRANSFORM PROTO_SRCS REPLACE "\\.proto" ".pb.cc")

set(PROTO_GRPC_HDRS "${PROTOS}")
set(PROTO_GRPC_SRCS "${PROTOS}")
list(TRANSFORM PROTO_GRPC_HDRS REPLACE "\\.proto" ".grpc.pb.h")
list(TRANSFORM PROTO_GRPC_SRCS REPLACE "\\.proto" ".grpc.pb.cc")

get_property(GRPC_CPP_PLUGIN TARGET gRPC::grpc_cpp_plugin PROPERTY LOCATION)

add_custom_command(
  OUTPUT  ${PROTO_SRCS} ${PROTO_HDRS} ${PROTO_GRPC_SRCS} ${PROTO_GRPC_HDRS}
  COMMAND protobuf::protoc
  ARGS    --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_BINARY_DIR}
          --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
          --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
          ${PROTOS}
  COMMENT "Running Protobuf / gRPC C++ Compiler"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  VERBATIM
  DEPENDS ${PROTOS})
add_library(protos ${PROTO_SRCS} ${PROTO_HDRS} ${PROTO_GRPC_SRCS} ${PROTO_GRPC_HDRS})
target_include_directories(protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(protos protobuf::libprotobuf gRPC::grpc++)
