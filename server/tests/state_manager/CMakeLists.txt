add_executable(test_state_manager test_state_manager.cpp)
target_link_libraries(test_state_manager PRIVATE 
  Catch2::Catch2 
  Catch2::Catch2WithMain
  spdlog::spdlog
  beatled_state_manager 
)
catch_discover_tests(test_state_manager)
