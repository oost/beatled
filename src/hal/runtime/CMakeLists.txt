# if(POSIX_PORT AND APPLE)
#   add_subdirectory(simulator)
# else() 
#   add_subdirectory(standard)
# endif()


# set(RUNTIME_SOURCES ${RUNTIME_SOURCES} PARENT_SCOPE)
# set(RUNTIME_LIBRARIES ${RUNTIME_LIBRARIES} PARENT_SCOPE)

add_hal_module(beatled_hal_runtime)
