add_library(beatled_clock INTERFACE )

target_sources(
  beatled_clock INTERFACE 
      clock.c
)

target_link_libraries(beatled_clock INTERFACE
  beatled_hal_blink
  beatled_hal_time
)
target_include_directories(beatled_clock INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
