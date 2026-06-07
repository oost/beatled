target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  ws2812.cpp
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  beatled_hal_runtime
)