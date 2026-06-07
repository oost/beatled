add_executable(test_event_queue test_event_queue.cpp)
target_include_directories(test_event_queue PRIVATE
  ${CMAKE_SOURCE_DIR}/src/state_manager/include
  ${CMAKE_SOURCE_DIR}/src/config/include
)
target_link_libraries(test_event_queue PRIVATE
  Catch2::Catch2WithMain
  beatled_event
  beatled_hal_time
  beatled_hal_board
  $<$<BOOL:${USE_FREERTOS}>:freertos_test_hooks>
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_event_queue)
endif()
