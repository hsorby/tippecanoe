
cmake_minimum_required(VERSION 3.21.0)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0 CACHE STRING "Minimum OS X deployment version.")
endif()

set(PROJECT_NAME clipper2)
set(CURRENT_VERSION 0.7.0)

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION} LANGUAGES CXX)

add_library(${PROJECT_NAME} src/clipper.engine.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

