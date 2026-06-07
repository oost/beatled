target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    blink.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  pico_stdlib
  ${PICO_WIFI_LIBRARY}
)

# For lwipopts.h
target_include_directories(${CURRENT_LIBRARY_NAME} INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/../../..
)
