
# These tests can use the Catch2-provided main
add_executable(test_queue test_queue.cpp)
target_include_directories(test_queue PRIVATE
  ${CMAKE_SOURCE_DIR}/src/config/include
)
target_link_libraries(test_queue PRIVATE
  Catch2::Catch2WithMain
  beatled_hal_queue
  $<$<BOOL:${USE_FREERTOS}>:freertos_test_hooks>
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_queue)
endif()