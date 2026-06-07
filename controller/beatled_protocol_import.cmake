# This is a copy of <PICO_SDK_PATH>/external/pico_sdk_import.cmake

# This can be dropped into an external project to help locate this SDK
# It should be include()ed prior to project()

if (DEFINED ENV{BEATLED_PROTOCOL_PATH} AND (NOT BEATLED_PROTOCOL_PATH))
    set(BEATLED_PROTOCOL_PATH $ENV{BEATLED_PROTOCOL_PATH})
    message("Using BEATLED_PROTOCOL_PATH from environment ('${BEATLED_PROTOCOL_PATH}')")
endif ()


set(BEATLED_PROTOCOL_PATH "${BEATLED_PROTOCOL_PATH}" CACHE PATH "Path to the Beatled Protocol package")


if (NOT BEATLED_PROTOCOL_PATH)
    message(FATAL_ERROR
            "SDK location was not specified. Please set BEATLED_PROTOCOL_PATH."
            )
    
endif ()

get_filename_component(BEATLED_PROTOCOL_PATH "${BEATLED_PROTOCOL_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
if (NOT EXISTS ${BEATLED_PROTOCOL_PATH})
    message(FATAL_ERROR "Directory '${BEATLED_PROTOCOL_PATH}' not found")
endif ()

set(BEATLED_PROTOCOL_INIT_CMAKE_FILE ${BEATLED_PROTOCOL_PATH}/beatled_protocol_init.cmake)
if (NOT EXISTS ${BEATLED_PROTOCOL_INIT_CMAKE_FILE})
    message(FATAL_ERROR "Directory '${BEATLED_PROTOCOL_PATH}' does not appear to contain the Beatled Protocol package")
endif ()

set(BEATLED_PROTOCOL_PATH ${BEATLED_PROTOCOL_PATH} CACHE PATH "Path to the Beatled protocol" FORCE)

include(${BEATLED_PROTOCOL_INIT_CMAKE_FILE})
