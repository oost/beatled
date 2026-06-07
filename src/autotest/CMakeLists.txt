add_library(beatled_autotest INTERFACE)

target_sources(
  beatled_autotest INTERFACE 
    autotest.c
)

target_link_libraries(beatled_autotest INTERFACE
  beatled_protocol
  beatled_clock
  beatled_hal_network
  beatled_event
)

target_include_directories(beatled_autotest INTERFACE ${CMAKE_CURRENT_LIST_DIR})


