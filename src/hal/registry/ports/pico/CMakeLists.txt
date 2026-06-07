target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  registry.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  pico_sync
)
