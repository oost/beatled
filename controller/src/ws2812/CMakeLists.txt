
add_library(beatled_ws2812 INTERFACE)

# generate the header file into the source tree as it is included in the RP2040 datasheet

target_sources(beatled_ws2812 INTERFACE
  ws2812.c
  ws2812_patterns.c
  programs/snake/snake.c
  programs/drops/drops.c
  programs/fade/fade.c
  programs/random/random.c
  programs/greys/greys.c
  programs/solid/solid.c
  programs/sparkle/sparkle.c
)

target_link_libraries(beatled_ws2812 INTERFACE
        beatled_hal_ws2812
)

target_include_directories(beatled_ws2812 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)


# pico_add_extra_outputs(pio_ws2812)


# target_include_directories(ws2812 INTERFACE ${CMAKE_CURRENT_LIST_DIR})


