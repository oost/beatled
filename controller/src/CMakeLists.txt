# Add executable. Default name is the project name, version 0.1

message("Building beatled with ${CMAKE_BUILD_TYPE}")

add_subdirectory(autotest)
add_subdirectory(hal)
add_subdirectory(ws2812)
add_subdirectory(state_manager)
add_subdirectory(command)
add_subdirectory(clock)
add_subdirectory(event)
add_subdirectory(process)
add_subdirectory(config)


set(CURRENT_TARGET "pico_w_beatled")

add_executable(${CURRENT_TARGET}
  main.c
)
add_dependencies(${CURRENT_TARGET} beatled_version_header)

message("Compiling with WIFI_SSID ${WIFI_SSID} and password ${WIFI_PASSWORD}, BEATLED_SERVER_NAME=${BEATLED_SERVER_NAME} ")


target_compile_definitions(${CURRENT_TARGET}  PRIVATE
  WIFI_SSID=\"${WIFI_SSID}\"
  WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
  WIFI_SSID_2=\"${WIFI_SSID_2}\"
  WIFI_PASSWORD_2=\"${WIFI_PASSWORD_2}\"
  WIFI_SSID_3=\"${WIFI_SSID_3}\"
  WIFI_PASSWORD_3=\"${WIFI_PASSWORD_3}\"
  WIFI_SSID_4=\"${WIFI_SSID_4}\"
  WIFI_PASSWORD_4=\"${WIFI_PASSWORD_4}\"
  HOTSPOT_SSID=\"${HOTSPOT_SSID}\"
  HOTSPOT_PASSWORD=\"${HOTSPOT_PASSWORD}\"
  BEATLED_SERVER_NAME="${BEATLED_SERVER_NAME}"
)

target_include_directories(${CURRENT_TARGET} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(${CURRENT_TARGET} 
  ${RUNTIME_LIBRARIES}
  beatled_ws2812
  beatled_state_manager
  beatled_commands
  beatled_protocol
  beatled_clock
  beatled_hal
  beatled_process
  beatled_autotest
  beatled_config
)

if (PICO_PORT)
  if (NOT PICO_CYW43_SUPPORTED AND NOT TARGET pico_cyw43_arch)
      message(FATAL_ERROR "Skipping Pico W examples as support is not available ${PICO_CYW43_SUPPORTED}")
  endif()
  # generate the header file into the source tree as it is included in the RP2040 datasheet
  # pico_generate_pio_header(pico_w_beatled ${CMAKE_CURRENT_LIST_DIR}/ws2812/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

  if (PICO_PROBE)
    message("Configuring Pico to be used with Pico Probe (UART output)")

  else()
    message("Configuring Pico to be used with USB output")
  endif()
  
  pico_enable_stdio_uart(${CURRENT_TARGET}  PICO_PROBE)
  pico_enable_stdio_usb(${CURRENT_TARGET}  $<NOT:PICO_PROBE>)

  pico_add_extra_outputs(${CURRENT_TARGET} )

  pico_set_program_name(${CURRENT_TARGET}  "pico_w_beatled")
  pico_set_program_version(${CURRENT_TARGET}  "0.1")


  print_memsize(${PROJECT_NAME})
  target_link_libraries(${CURRENT_TARGET} pico_runtime)

elseif(POSIX_PORT AND APPLE)
  # set_target_properties(${CURRENT_TARGET} PROPERTIES )            
  set_target_properties(${CURRENT_TARGET}  PROPERTIES
      MACOSX_BUNDLE True
      BUNDLE True
      MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.${CURRENT_TARGET} 
      MACOSX_BUNDLE_BUNDLE_NAME ${CURRENT_TARGET} 
      MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/customtemplate.plist.in
  )
endif()



