message("Building FreeRTOS POSIX simulator")

SET(POSIX_RUNTIME_DIR ${CMAKE_CURRENT_LIST_DIR}/../posix)

SET(RESOURCE_COMPILER xxd)
FILE(GLOB_RECURSE SHADER_FILES "${POSIX_RUNTIME_DIR}/shaders/*")
FOREACH(INPUT_FILE ${SHADER_FILES})
    SET(INPUT_FILE_RELATIVE $<PATH:RELATIVE_PATH,${INPUT_FILE},${POSIX_RUNTIME_DIR}>)
    SET(OUTPUT_FILE "${INPUT_FILE_RELATIVE}.h")
    message(${OUTPUT_FILE})
    add_custom_command(
          OUTPUT ${OUTPUT_FILE}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/$<PATH:GET_PARENT_PATH,${OUTPUT_FILE}>
          COMMAND xxd -i ${INPUT_FILE_RELATIVE} ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}
          WORKING_DIRECTORY ${POSIX_RUNTIME_DIR}
          DEPENDS ${INPUT_FILE}
    )
    LIST(APPEND SHADER_HEADER_FILES ${OUTPUT_FILE})
ENDFOREACH()

# Embed binary assets (e.g. the top-hat OBJ) into headers, mirroring the posix
# port so the shared renderer.cpp finds assets/top_hat.obj.h here too.
FILE(GLOB_RECURSE ASSET_FILES "${POSIX_RUNTIME_DIR}/assets/*.obj")
FOREACH(INPUT_FILE ${ASSET_FILES})
    SET(INPUT_FILE_RELATIVE $<PATH:RELATIVE_PATH,${INPUT_FILE},${POSIX_RUNTIME_DIR}>)
    SET(OUTPUT_FILE "${INPUT_FILE_RELATIVE}.h")
    message(${OUTPUT_FILE})
    add_custom_command(
          OUTPUT ${OUTPUT_FILE}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/$<PATH:GET_PARENT_PATH,${OUTPUT_FILE}>
          COMMAND xxd -i ${INPUT_FILE_RELATIVE} ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}
          WORKING_DIRECTORY ${POSIX_RUNTIME_DIR}
          DEPENDS ${INPUT_FILE}
    )
    LIST(APPEND SHADER_HEADER_FILES ${OUTPUT_FILE})
ENDFOREACH()

add_custom_target(
  ShaderHeaders
  DEPENDS ${SHADER_HEADER_FILES}
)

target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/startup.cpp
  ${POSIX_RUNTIME_DIR}/renderer.cpp
  ${POSIX_RUNTIME_DIR}/mtk_view_delegate.cpp
  ${POSIX_RUNTIME_DIR}/app_delegate.cpp
  ${POSIX_RUNTIME_DIR}/math.cpp
  ${POSIX_RUNTIME_DIR}/mesh_loader.cpp
  ${POSIX_RUNTIME_DIR}/led_buffer.cpp
  ${POSIX_RUNTIME_DIR}/status_buffer.cpp
  ${POSIX_RUNTIME_DIR}/overlay_renderer.cpp
)

# tinyobjloader (mesh loading) comes from the controller vcpkg manifest; the
# shared ../posix/mesh_loader.cpp needs it here too.
find_package(tinyobjloader CONFIG REQUIRED)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  METAL_CPP
  tinyobjloader::tinyobjloader
  "-framework CoreGraphics"
  "-framework CoreText"
  FreeRTOS-Kernel
  FreeRTOS-Kernel-Heap4
)

add_dependencies(${CURRENT_LIBRARY_NAME} ShaderHeaders)

target_include_directories(${CURRENT_LIBRARY_NAME} INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}
)
