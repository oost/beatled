if (PICO_PORT)
  add_executable(test_udp_send 
      udp_send.c
  )

  target_compile_definitions(test_udp_send PRIVATE
      WIFI_SSID=\"${WIFI_SSID}\"
      WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
      BEATLED_SERVER_NAME=\"${SERVER_NAME}\"
  )


  pico_add_extra_outputs(test_udp_send)


  pico_set_program_name(test_udp_send "test_udp_send")
  pico_set_program_version(test_udp_send "0.1")

  pico_enable_stdio_uart(test_udp_send PICO_PROBE)
  pico_enable_stdio_usb(test_udp_send $<NOT:PICO_PROBE>)

  # target_include_directories(pico_ws2812 PRIVATE ${CMAKE_CURRENT_LIST_DIR})

  # Add any user requested libraries
  target_link_libraries(
      test_udp_send
      pico_stdlib 
      pico_cyw43_arch_lwip_threadsafe_background

  )

  target_include_directories(
      test_udp_send PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}
          ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
  )
endif()