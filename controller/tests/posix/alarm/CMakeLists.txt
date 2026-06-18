
# The alarm test waits with an OS sleep and expects preemptive timer callbacks
# to fire meanwhile. That model only holds on the plain POSIX port (OS timers on
# their own threads). On the cooperative FreeRTOS-POSIX simulator, software
# timers fire from the timer daemon task, which needs the scheduler running and
# a vTaskDelay-style yield — incompatible with this OS-sleep test, and wrapping
# the Catch2 session in a started scheduler deadlocks (Catch2's signal handlers
# clash with the POSIX port's signal-based ticking). The FreeRTOS timer port is
# exercised by the freertos-sim app at runtime; the alarm HAL contract is
# covered here on the POSIX port. So build this test only for non-FreeRTOS.
if(NOT USE_FREERTOS)
  add_executable(test_alarm test_alarm.cpp)
  target_include_directories(test_alarm PRIVATE
    ${CMAKE_SOURCE_DIR}/src/config/include
  )
  target_link_libraries(test_alarm PRIVATE
    Catch2::Catch2WithMain
    beatled_hal_time
  )
  if(NOT VCPKG_TARGET_TRIPLET)
    catch_discover_tests(test_alarm)
  endif()
endif()
