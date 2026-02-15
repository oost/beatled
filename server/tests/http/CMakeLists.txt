add_executable(test_mime_types test_mime_types.cpp)
target_link_libraries(test_mime_types PRIVATE
  Catch2::Catch2
  Catch2::Catch2WithMain
  beatled_http_server
  spdlog::spdlog
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_mime_types)
endif()
