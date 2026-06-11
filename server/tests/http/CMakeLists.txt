add_executable(test_mime_types test_mime_types.cpp)
target_link_libraries(test_mime_types PRIVATE
  Catch2::Catch2WithMain
  beatled_http_server
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_mime_types)
endif()

add_executable(test_tls_context test_tls_context.cpp)
target_link_libraries(test_tls_context PRIVATE
  Catch2::Catch2WithMain
  beatled_http_server
)
target_compile_definitions(test_tls_context PRIVATE
  TEST_FIXTURES_DIR="${CMAKE_CURRENT_LIST_DIR}/fixtures"
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_tls_context)
endif()

add_executable(test_api_gates test_api_gates.cpp)
target_link_libraries(test_api_gates PRIVATE
  Catch2::Catch2WithMain
  beatled_http_server
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_api_gates)
endif()
