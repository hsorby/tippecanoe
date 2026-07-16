
cmake_minimum_required(VERSION 3.21.0)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0 CACHE STRING "Minimum OS X deployment version.")
endif()

set(PROJECT_NAME jsonpull)
set(CURRENT_VERSION 0.1.0)

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION} LANGUAGES C)

add_library(${PROJECT_NAME} jsonpull.c)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

