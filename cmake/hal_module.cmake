macro(add_hal_module MODULE_NAME)
  set(CURRENT_LIBRARY_NAME ${MODULE_NAME})
  message("Adding ${CURRENT_LIBRARY_NAME} HAL module")

  add_library(${CURRENT_LIBRARY_NAME} INTERFACE)

  target_include_directories(${CURRENT_LIBRARY_NAME} INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/include
  )

  add_subdirectory(ports/${PORT})
endmacro()