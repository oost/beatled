# Pre-initialize the Raspberry Pi Pico SDK, setting up the platform and toolchain and some CMake utility functions
# This file must be included prior to the project() call

# Note: this file is perhaps named badly, as it provides a method pico_sdk_init which
# the enclosing project calls LATER to actually "initialize" the SDK (by including the CMakeLists.txt from this
# same directory)

if (NOT TARGET _beatled_protocol_pre_init_marker)
    add_library(_beatled_protocol_pre_init_marker INTERFACE)

    function(beatled_protocol_is_top_level_project VAR)
        string(TOLOWER ${CMAKE_CURRENT_LIST_DIR} __list_dir)
        string(TOLOWER ${CMAKE_SOURCE_DIR} __source_dir)
        if (__source_dir STREQUAL __list_dir)
            set(${VAR} 1 PARENT_SCOPE)
        else()
            set(${VAR} 0 PARENT_SCOPE)
        endif()
    endfunction()

    function(beatled_protocol_message_debug MESSAGE)
        # The log-level system was added in CMake 3.15.
        if(${CMAKE_VERSION} VERSION_LESS "3.15.0")
            message(${MESSAGE})
        else()
            message(DEBUG ${MESSAGE})
        endif()
    endfunction()

    if (NOT BEATLED_PROTOCOL_PATH)
        set(BEATLED_PROTOCOL_PATH ${CMAKE_CURRENT_LIST_DIR})
    endif ()

    get_filename_component(BEATLED_PROTOCOL_PATH "${BEATLED_PROTOCOL_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

    set(BEATLED_PROTOCOL_PATH ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)

    list(APPEND CMAKE_MODULE_PATH ${BEATLED_PROTOCOL_PATH}/cmake)

    include(${CMAKE_CURRENT_LIST_DIR}/beatled_protocol_version.cmake)
    # include(pico_utils)

    message("BEATLED_PROTOCOL_PATH is ${CMAKE_CURRENT_LIST_DIR}")

    # include(pico_pre_load_platform)

    # We want to configure correct toolchain prior to project load
    # todo perhaps this should be included by the platform instead?
    # include(pico_pre_load_toolchain)

    macro(beatled_protocol_init)
        if (NOT CMAKE_PROJECT_NAME)
            message(WARNING "beatled_protocol_init() should be called after the project is created (and languages added)")
        endif()
        add_subdirectory(${BEATLED_PROTOCOL_PATH} beatled_protocol)
    endmacro()

    macro(add_sub_list_dirs var)
        foreach(LIST_DIR IN LISTS ${var})
            get_filename_component(SHORT_NAME "${LIST_DIR}" NAME)
            beatled_protocol_message_debug("Including custom CMakeLists.txt ${SHORT_NAME}")
            add_subdirectory(${LIST_DIR} ${SHORT_NAME})
        endforeach()
    endmacro()

    macro(add_sub_list_files var)
        foreach(LIST_FILE IN LISTS ${var})
            beatled_protocol_message_debug("Including custom CMake file ${LIST_FILE}")
            include(${LIST_FILE})
        endforeach()
    endmacro()

    macro(beatled_protocol_register_common_scope_var NAME)
        if (NOT ${NAME} IN_LIST PICO_PROMOTE_COMMON_SCOPE_VARS)
            list(APPEND PICO_PROMOTE_COMMON_SCOPE_VARS ${NAME})
        endif()
    endmacro()

    set(PICO_PROMOTE_COMMON_SCOPE_VARS
            PICO_INCLUDE_DIRS
            PICO_SDK_POST_LIST_DIRS
            PICO_SDK_POST_LIST_FILES
            PICO_CONFIG_HEADER_FILES
            PICO_RP2040_CONFIG_HEADER_FILES
    )

    macro(beatled_protocol_promote_common_scope_vars)
        set(PICO_PROMOTE_COMMON_SCOPE_VARS ${PICO_PROMOTE_COMMON_SCOPE_VARS} PARENT_SCOPE)
        foreach(VAR IN LISTS PICO_PROMOTE_COMMON_SCOPE_VARS)
            SET(${VAR} ${${VAR}} PARENT_SCOPE)
        endforeach()
    endmacro()
endif()
