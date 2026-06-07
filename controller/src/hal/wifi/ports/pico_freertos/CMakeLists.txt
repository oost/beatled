target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    wifi.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
    ${PICO_WIFI_LIBRARY}
)

target_include_directories(${CURRENT_LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../../..)
