add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE
  Catch2::Catch2WithMain
  beatled_protocol
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_protocol)
endif()
