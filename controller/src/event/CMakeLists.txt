add_library(beatled_event INTERFACE)

target_sources(
    beatled_event INTERFACE 
    event_queue.c
    event_loop.c
)

target_link_libraries(beatled_event INTERFACE
    beatled_hal_queue
    beatled_hal_process
)


# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "dir='${dir}'")
# endforeach()

target_include_directories(beatled_event INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
