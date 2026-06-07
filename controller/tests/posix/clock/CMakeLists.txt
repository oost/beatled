
# These tests can use the Catch2-provided main
add_executable(test_clock test_clock.cpp)
target_link_libraries(test_clock PRIVATE
  Catch2::Catch2WithMain
  beatled_commands
  beatled_ws2812
  $<$<BOOL:${USE_FREERTOS}>:freertos_test_hooks>
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_clock)
endif()