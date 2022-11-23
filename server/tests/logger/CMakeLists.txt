add_executable(test_fmt_log test_fmt_log.cpp)
target_link_libraries(test_fmt_log PRIVATE 
  Catch2::Catch2 
  Catch2::Catch2WithMain
  spdlog::spdlog
  beatled_udp  
)
catch_discover_tests(test_fmt_log)
