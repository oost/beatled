add_executable(test_tempo_broadcaster test_tempo_broadcaster.cpp)
target_link_libraries(test_tempo_broadcaster PRIVATE
  Catch2::Catch2WithMain
  beatled_tempo_broadcaster
  beatled_core
)
if(NOT VCPKG_TARGET_TRIPLET)
  catch_discover_tests(test_tempo_broadcaster)
endif()
