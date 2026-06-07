#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

#include "event/event_queue.h"
#include "state_manager/state_manager.h"

#ifdef __cplusplus
}
#endif

TEST_CASE("Event queue operations", "[event_queue]") {
  event_queue_init();

  SECTION("Server message event can be added and popped") {
    int payload = 42;
    void *data = malloc(sizeof(int));
    REQUIRE(data != nullptr);
    memcpy(data, &payload, sizeof(int));

    bool added =
        event_queue_add_message(event_server_message, data, sizeof(int));
    REQUIRE(added);

    event_t event;
    event_queue_pop_message_blocking(&event);

    REQUIRE(event.event_type == event_server_message);
    REQUIRE(event.data == data);
    REQUIRE(event.data_length == sizeof(int));
    REQUIRE(*(int *)event.data == 42);

    free(event.data);
  }

  SECTION("State transition event can be added and popped") {
    state_event_t *state_event =
        (state_event_t *)malloc(sizeof(state_event_t));
    REQUIRE(state_event != nullptr);
    state_event->next_state = STATE_REGISTERED;

    bool added = event_queue_add_message(event_state_transition, state_event,
                                         sizeof(state_event_t));
    REQUIRE(added);

    event_t event;
    event_queue_pop_message_blocking(&event);

    REQUIRE(event.event_type == event_state_transition);
    state_event_t *popped = (state_event_t *)event.data;
    REQUIRE(popped->next_state == STATE_REGISTERED);

    free(event.data);
  }

  SECTION("add_payload_to_event_queue wrapper returns 0 on success") {
    void *data = malloc(8);
    REQUIRE(data != nullptr);

    int result = add_payload_to_event_queue(data, 8, /*rx_time_us=*/1234567);
    REQUIRE(result == 0);

    // Drain the event
    event_t event;
    event_queue_pop_message_blocking(&event);
    REQUIRE(event.event_type == event_server_message);
    free(event.data);
  }

  SECTION("Events carry a non-zero timestamp") {
    void *data = malloc(4);
    REQUIRE(data != nullptr);

    event_queue_add_message(event_server_message, data, 4);

    event_t event;
    event_queue_pop_message_blocking(&event);
    REQUIRE(event.time > 0);

    free(event.data);
  }

  SECTION("Multiple events maintain FIFO order") {
    int *data1 = (int *)malloc(sizeof(int));
    int *data2 = (int *)malloc(sizeof(int));
    int *data3 = (int *)malloc(sizeof(int));
    REQUIRE(data1 != nullptr);
    REQUIRE(data2 != nullptr);
    REQUIRE(data3 != nullptr);
    *data1 = 1;
    *data2 = 2;
    *data3 = 3;

    event_queue_add_message(event_server_message, data1, sizeof(int));
    event_queue_add_message(event_state_transition, data2, sizeof(int));
    event_queue_add_message(event_error, data3, sizeof(int));

    event_t event;

    event_queue_pop_message_blocking(&event);
    REQUIRE(*(int *)event.data == 1);
    REQUIRE(event.event_type == event_server_message);
    free(event.data);

    event_queue_pop_message_blocking(&event);
    REQUIRE(*(int *)event.data == 2);
    REQUIRE(event.event_type == event_state_transition);
    free(event.data);

    event_queue_pop_message_blocking(&event);
    REQUIRE(*(int *)event.data == 3);
    REQUIRE(event.event_type == event_error);
    free(event.data);
  }
}
