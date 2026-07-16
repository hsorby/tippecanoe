
cmake_minimum_required(VERSION 3.24.0)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0 CACHE STRING "Minimum OS X deployment version.")
endif()

set(PROJECT_NAME tippecanoe)
set(CURRENT_VERSION 2.80.0)

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION} LANGUAGES CXX)

include(FetchContent)

if (WIN32)
  FetchContent_Declare(
    zlib
    URL https://github.com/madler/zlib/releases/download/v1.3.2/zlib132.zip
  )

  FetchContent_MakeAvailable(zlib)
else()
  find_package(ZLIB REQUIRED)
endif()

if(NOT TARGET ZLIB::ZLIB)
  message(FATAL_ERROR "ZLIB::ZLIB target not available. Tippecanoe requires this target for building the application.")
endif()

if(WIN32)
  FetchContent_Declare(
    sqlite3
    URL https://sqlite.org/2026/sqlite-amalgamation-3530300.zip
  )

  FetchContent_MakeAvailable(sqlite3)

  add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
  target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})

  add_library(SQLite3::SQLite3 ALIAS sqlite3)
else()
  find_package(SQlite3 REQUIRED)
endif()

if(NOT TARGET SQLite3::SQLite3) # CMake < 4.3
  add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()

find_package(Threads REQUIRED)
if(NOT TARGET Threads::Threads)
  message(FATAL_ERROR "Threads::Threads target not available. Tippecanoe requires this target for building the application.")
endif()

add_subdirectory(clipper2)
add_subdirectory(jsonpull)

set(_SRCS
  geojson.cpp
  tile.cpp
  pool.cpp
  mbtiles.cpp
  geometry.cpp
  projection.cpp
  memfile.cpp
  mvt.cpp
  serial.cpp
  main.cpp
  platform.cpp
  text.cpp
  dirtiles.cpp
  pmtiles_file.cpp
  plugin.cpp
  read_json.cpp
  write_json.cpp
  geobuf.cpp
  flatgeobuf.cpp
  evaluator.cpp
  geocsv.cpp
  csv.cpp
  geojson-loop.cpp
  json_logger.cpp
  visvalingam.cpp
  compression.cpp
  clip.cpp
  sort.cpp
  attribute.cpp
  thread.cpp
  shared_borders.cpp
)

add_executable(${PROJECT_NAME} ${_SRCS})
#target_include_directories(${PROJECT_NAME} PUBLIC include)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC BUILD_INFO=$<CONFIG>)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC clipper2 jsonpull SQLite3::SQLite3 ZLIB::ZLIB Threads::Threads)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
include(CheckPIESupported)
check_pie_supported()
