# Compile the pattern sources directly: linking beatled_ws2812 would pull
# in ws2812.c and its registry/intercore dependencies, which the pure
# pattern functions don't need.
set(WS2812_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../src/ws2812)
add_executable(test_patterns
  test_patterns.cpp
  ${WS2812_DIR}/ws2812_patterns.c
  ${WS2812_DIR}/programs/snake/snake.c
  ${WS2812_DIR}/programs/drops/drops.c
  ${WS2812_DIR}/programs/fade/fade.c
  ${WS2812_DIR}/programs/random/random.c
  ${WS2812_DIR}/programs/greys/greys.c
  ${WS2812_DIR}/programs/off/off.c
  ${WS2812_DIR}/programs/solid/solid.c
  ${WS2812_DIR}/programs/sparkle/sparkle.c
)
target_include_directories(test_patterns PRIVATE ${WS2812_DIR})
target_link_libraries(test_patterns PRIVATE
  Catch2::Catch2WithMain
  beatled_protocol
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_patterns)
endif()
