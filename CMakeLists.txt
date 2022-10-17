cmake_minimum_required(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

# project name
project(libNeo6m)

# require
set(CMAKE_CXX_STANDARD 20)
#
# generates commpile_commands.json for LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

# enable testing
enable_testing()

include_directories(include)
# fix for compiler error relating to pthread
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")

#add_library(<name> <wheretofind>)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(tests)

# include_directories(${PROJECT_SOURCE_DIR}/include)
