MESSAGE("${CMAKE_CXX_STANDARD}")
MESSAGE("${INCLUDE_DIRECTORIES}")
add_library(libNeo6m STATIC libNeo6m.cpp)
# target_include_directories(libNeo6m PUBLIC ${CMAKE_SOURCE_DIR}/include)
