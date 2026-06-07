
add_executable(test_alarm test_alarm.cpp)
target_include_directories(test_alarm PRIVATE
  ${CMAKE_SOURCE_DIR}/src/config/include
)
target_link_libraries(test_alarm PRIVATE
  Catch2::Catch2WithMain
  beatled_hal_time
  $<$<BOOL:${USE_FREERTOS}>:freertos_test_hooks>
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_alarm)
endif()
