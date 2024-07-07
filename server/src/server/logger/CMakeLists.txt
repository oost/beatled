add_library(beatled_logger 
  logger.cpp 
)

target_link_libraries(beatled_logger PUBLIC 
  nlohmann_json::nlohmann_json 
  date::date 
  date::date-tz
)

target_include_directories(beatled_logger PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)