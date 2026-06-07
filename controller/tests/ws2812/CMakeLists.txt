# Add executable. Default name is the project name, version 0.1

if (PICO_PORT)

    add_executable(pico_ws2812 
            main.c
    )

    pico_generate_pio_header(pico_ws2812 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

    pico_add_extra_outputs(pico_ws2812)


    pico_set_program_name(pico_ws2812 "pico_ws2812")
    pico_set_program_version(pico_ws2812 "0.1")

    pico_enable_stdio_uart(pico_ws2812 0)
    pico_enable_stdio_usb(pico_ws2812 1)

    # target_include_directories(pico_ws2812 PRIVATE ${CMAKE_CURRENT_LIST_DIR})

    # Add any user requested libraries
    target_link_libraries(pico_ws2812
        pico_stdlib 
        hardware_pio 
        hardware_dma
    )


    print_memsize(pico_ws2812)

    add_executable(pico_ws2812_parallel
        main_parallel.c
    )

    pico_generate_pio_header(pico_ws2812_parallel ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

    pico_add_extra_outputs(pico_ws2812_parallel)


    pico_set_program_name(pico_ws2812_parallel "pico_ws2812_p")
    pico_set_program_version(pico_ws2812_parallel "0.1")

    pico_enable_stdio_uart(pico_ws2812_parallel 0)
    pico_enable_stdio_usb(pico_ws2812_parallel 1)

    # target_include_directories(pico_ws2812 PRIVATE ${CMAKE_CURRENT_LIST_DIR})

    # Add any user requested libraries
    target_link_libraries(pico_ws2812_parallel
        pico_stdlib 
        hardware_pio 
        hardware_dma
    )

    print_memsize(pico_ws2812_parallel)

    # DMA 
    add_executable(pico_ws2812_dma
        main_dma.c
    )

    pico_generate_pio_header(pico_ws2812_dma ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

    pico_add_extra_outputs(pico_ws2812_dma)


    pico_set_program_name(pico_ws2812_dma "pico_ws2812_dma")
    pico_set_program_version(pico_ws2812_dma "0.1")

    pico_enable_stdio_uart(pico_ws2812_dma 0)
    pico_enable_stdio_usb(pico_ws2812_dma 1)

    # target_include_directories(pico_ws2812 PRIVATE ${CMAKE_CURRENT_LIST_DIR})

    # Add any user requested libraries
    target_link_libraries(pico_ws2812_dma
    pico_stdlib 
    hardware_pio 
    hardware_dma
    )

    print_memsize(pico_ws2812_dma)

endif()