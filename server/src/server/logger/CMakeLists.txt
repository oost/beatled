add_library(logger logger.cpp )
target_link_libraries(logger PRIVATE nlohmann_json::nlohmann_json date::date date::date-tz)
