include(Buf)

set(AXY_SDIFI_PROTO_REPOSITORY
  "https://github.com/sdifi/protos.git#branch=master"
  CACHE STRING
  "Repository for fetching SDiFI protos"
)

set(TIRO_SPEECH_PROTO_REPOSITORY
  "https://github.com/tiro-is/tiro-speech-core.git#branch=master,subdir=proto"
  CACHE STRING
  "Repository for fetching Tiro Speech protos"
)

buf_generate_sources(
  OUTPUT PROTO_SRCS_HDRS
  REPOSITORIES ${TIRO_SPEECH_PROTO_REPOSITORY} ${AXY_SDIFI_PROTO_REPOSITORY}
  INCLUDE_IMPORTS
)

add_library(protos ${PROTO_SRCS_HDRS})
target_include_directories(protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
