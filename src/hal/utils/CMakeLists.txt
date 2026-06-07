set(CURRENT_LIBRARY_NAME beatled_hal_utils)
message("Adding ${CURRENT_LIBRARY_NAME} HAL module")

add_library(${CURRENT_LIBRARY_NAME} INTERFACE)

target_include_directories(${CURRENT_LIBRARY_NAME}
  INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include
)