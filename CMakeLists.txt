cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(OPT_WARNINGS_AS_ERRORS_DEVELOPER_DEFAULT TRUE)

project(
  axy
  VERSION 0.0.1
  DESCRIPTION ""
  HOMEPAGE_URL "https://github.com/SDifI/axy"
  LANGUAGES CXX C)

string(
  COMPARE EQUAL
          "${CMAKE_SOURCE_DIR}"
          "${PROJECT_SOURCE_DIR}"
          PROJECT_IS_TOP_LEVEL)
if(PROJECT_IS_TOP_LEVEL)
  include(CTest)
endif()

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")
string(
  SUBSTRING "${GIT_SHA}"
            0
            8
            GIT_SHORT_SHA)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

option(PREFER_STATIC "Prefer static libs" ON)
if(PREFER_STATIC)
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
endif()

option(ENABLE_SANITIZERS "Use UBSan and ASan in debug build" ${PROJECT_IS_TOP_LEVEL})

include(cmake/deps.cmake)

add_subdirectory(configured_files)
add_subdirectory(proto)
add_subdirectory(src)

if(CMAKE_SKIP_INSTALL_RULES)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()

set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
