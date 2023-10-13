# Add FetchContent deps as subdirectory since EXCLUDE_FROM_ALL arg only got
# added to FetchContent_Declare in 3.28. This has the same effect.
add_subdirectory(cmake/fetch EXCLUDE_FROM_ALL)

# The configuration phase for redis++ requires an installation of hiredis, so we
# can't use FetchContent to fetch hiredis
add_library(hiredis::hiredis UNKNOWN IMPORTED)
find_library(HIREDIS_LIB hiredis)
find_path(HIREDIS_HEADER hiredis)
set_target_properties(hiredis::hiredis PROPERTIES
  IMPORTED_LOCATION ${HIREDIS_LIB}
  INTERFACE_INCLUDE_DIRECTORIES ${HIREDIS_HEADER}
)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
