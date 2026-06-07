target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  time.c
  alarm.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  pico_stdlib
  pico_time
)
