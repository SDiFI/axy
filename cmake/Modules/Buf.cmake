find_program(BUF buf REQUIRED)

function(buf_generate_sources)
  cmake_parse_arguments(PARSE_ARGV 0 PROTO "INCLUDE_IMPORTS" "OUTPUT" "REPOSITORIES")
  set(seen_protos "")
  set(generated_srcs_and_hdrs "")

  string(REPLACE ";" "_" buf_seen_invoc_id ${PROTO_REPOSITORIES})
  if(NOT "${BUF_SEEN_INVOC_${buf_seen_invo_id}}" STREQUAL "")
    set(generated_srcs_and_hdrs ${BUF_SEEN_INVOC_${buf_seen_invo_id}})
  else()
    foreach(_repo ${PROTO_REPOSITORIES})
      message(STATUS "[Buf] getting proto file list from ${_repo}")

      set(include_imports_arg "")
      if(PROTO_INCLUDE_IMPORTS)
        set(include_imports_arg "--include-imports")
      endif()

      execute_process(
        COMMAND ${BUF} ls-files ${include_imports_arg} "${_repo}" --as-import-paths
        OUTPUT_VARIABLE protos
        COMMAND_ERROR_IS_FATAL ANY
      )
      string(REPLACE "\n" ";" protos ${protos})

      if("${protos}" STREQUAL "")
        message(FATAL_ERROR "[Buf] got no protos from ${_repo}")
      endif()

      # Exclude WKT, which are included in --include-imports
      list(FILTER protos EXCLUDE REGEX "google/protobuf")

      foreach(_seen_proto ${seen_protos})
        list(FILTER protos EXCLUDE REGEX "${_seen_proto}")
      endforeach()

      set(seen_protos ${seen_protos} ${protos})

      set(proto_hdrs ${protos})
      set(proto_srcs ${protos})
      list(TRANSFORM proto_hdrs REPLACE "\\.proto" ".pb.h")
      list(TRANSFORM proto_srcs REPLACE "\\.proto" ".pb.cc")

      set(proto_grpc_hdrs ${protos})
      set(proto_grpc_srcs ${protos})
      list(TRANSFORM proto_grpc_hdrs REPLACE "\\.proto" ".grpc.pb.h")
      list(TRANSFORM proto_grpc_srcs REPLACE "\\.proto" ".grpc.pb.cc")

      set(proto_generated
        ${proto_hdrs} ${proto_srcs} ${proto_grpc_hdrs} ${proto_grpc_srcs}
      )

      get_property(grpc_cpp_plugin TARGET gRPC::grpc_cpp_plugin PROPERTY LOCATION)

      add_custom_command(
        OUTPUT ${proto_generated}
        COMMAND ${BUF} generate "${_repo}" -o ${CMAKE_CURRENT_BINARY_DIR} ${include_imports_arg}
                --template=${CMAKE_SOURCE_DIR}
        COMMENT "[Buf] Generating gRPC code from ${_repo}"
        VERBATIM
      )

      set(generated_srcs_and_hdrs ${generated_srcs_and_hdrs} ${proto_generated})
    endforeach()
  endif()

  set(BUF_SEEN_REPOS "${PROTO_REPOSITORIES};${BUF_SEEN_REPOS}" CACHE INTERNAL "")
  set(BUF_SEEN_PROTOS "${PROTO_REPOSITORIES};${BUF_SEEN_REPOS}" CACHE INTERNAL "")

  set("BUF_SEEN_INVOC_${buf_seen_invo_id}" ${generated_srcs_and_hdrs} CACHE INTERNAL "")

  set(${PROTO_OUTPUT} ${generated_srcs_and_hdrs} PARENT_SCOPE)
endfunction()
