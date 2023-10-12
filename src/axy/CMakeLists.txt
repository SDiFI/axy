set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_library(axylib
  speech-service.cc speech-service.h
  server.cc         server.h
)

target_link_libraries(
  axylib
  PUBLIC
  redis++::redis++_static
  gRPC::grpc++
  gRPC::grpc++_reflection
  CLI11::CLI11
  fmt::fmt
  readerwriterqueue
  protos
)
target_include_directories(
  axylib
  PUBLIC
  "${CMAKE_BINARY_DIR}/configured_files/include"
  "${CMAKE_SOURCE_DIR}"
)

add_executable(axy
  main.cc
)
target_link_libraries(
  axy
  PRIVATE
  axylib
)


add_executable(test-client
  test-client.cc
)
target_link_libraries(
  test-client
  PRIVATE
  axylib
)
