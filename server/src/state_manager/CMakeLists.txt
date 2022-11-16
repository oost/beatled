add_library(beatled_state_manager state_manager.cpp)
# target_link_libraries(beatled_state_manager asio)

target_include_directories(beatled_state_manager PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
