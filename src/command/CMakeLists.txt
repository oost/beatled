add_library(beatled_commands INTERFACE)

target_sources(
  beatled_commands INTERFACE 
    command.c
    hello/hello.c
    time/time.c
    tempo/tempo.c
    next_beat/next_beat.c
)
target_link_libraries(beatled_commands INTERFACE
  beatled_hal
  beatled_protocol
  beatled_clock
  beatled_config
  beatled_event
  beatled_process
)


target_include_directories(beatled_commands INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
