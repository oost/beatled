add_executable(test_state_manager test_state_manager.cpp)
target_link_libraries(test_state_manager PRIVATE
  Catch2::Catch2
  Catch2::Catch2WithMain
  spdlog::spdlog
  beatled_core
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_state_manager)
endif()

add_executable(test_client_status_json test_client_status_json.cpp)
target_link_libraries(test_client_status_json PRIVATE
  Catch2::Catch2
  Catch2::Catch2WithMain
  spdlog::spdlog
  beatled_core
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_client_status_json)
endif()
