add_executable(test_api_handler test_api_handler.cpp)
target_link_libraries(test_api_handler PRIVATE
  Catch2::Catch2
  Catch2::Catch2WithMain
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  beatled_core
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_api_handler)
endif()
