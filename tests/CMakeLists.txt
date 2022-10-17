
find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)

# NEO6M ################################################################################
# testing executable
add_executable(test_neo6m test_neo6m.cpp)
# here we are adding the libraries we want to link the combiled binary test1 to
target_link_libraries(test_neo6m libNeo6m ${Boost_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_UNIT_TEST_FRAMWORK_LIBRARY})

#
target_include_directories(test_neo6m PRIVATE ${Boost_INCLUDE_DIRS})


# enable testing (needs to be done before every "add test")
enable_testing()

# add tests
add_test(NAME test_neo6mLib COMMAND test_neo6m)
