
# These tests can use the Catch2-provided main
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(tests)
endif()