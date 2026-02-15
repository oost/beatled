add_executable(test_udp_protocol test_udp_protocol.cpp)
target_link_libraries(test_udp_protocol PRIVATE
  Catch2::Catch2
  Catch2::Catch2WithMain
  spdlog::spdlog
  beatled_udp
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_udp_protocol)
endif()

add_executable(test_udp_request_handler test_udp_request_handler.cpp)
target_link_libraries(test_udp_request_handler PRIVATE
  Catch2::Catch2
  Catch2::Catch2WithMain
  spdlog::spdlog
  beatled_udp_server
  beatled_udp
  beatled_core
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_udp_request_handler)
endif()
