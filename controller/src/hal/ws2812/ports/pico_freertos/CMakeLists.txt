target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  ws2812.c
  ws2812_dma.c
)

pico_generate_pio_header(
  ${CURRENT_LIBRARY_NAME}
  ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
  OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  pico_stdlib
  hardware_pio
  hardware_dma
  FreeRTOS-Kernel
)
