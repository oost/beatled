#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

#include "beatled/protocol.h"
#include "command/command.h"
#include "event/event_queue.h"

// validate_server_message is not in the public header but has external linkage
int validate_server_message(void *event_data, size_t data_length);

#ifdef __cplusplus
}
#endif

TEST_CASE("Message validation", "[command]") {
  event_queue_init();

  SECTION("NULL data is rejected") {
    int result = validate_server_message(NULL, 10);
    REQUIRE(result == 1);
  }

  SECTION("Too-small data length is rejected") {
    beatled_message_t msg = {.type = BEATLED_MESSAGE_ERROR};
    // sizeof(beatled_message_t) is 1, so data_length must be > 1
    int result = validate_server_message(&msg, 0);
    REQUIRE(result == 1);

    result = validate_server_message(&msg, 1);
    REQUIRE(result == 1);
  }

  SECTION("HELLO_RESPONSE with correct size passes") {
    beatled_message_hello_response_t msg = {.base = {.type =
                                                         BEATLED_MESSAGE_HELLO_RESPONSE}};
    int result =
        validate_server_message(&msg, sizeof(beatled_message_hello_response_t));
    REQUIRE(result == 0);
  }

  SECTION("HELLO_RESPONSE with wrong size fails") {
    beatled_message_hello_response_t msg = {.base = {.type =
                                                         BEATLED_MESSAGE_HELLO_RESPONSE}};
    int result = validate_server_message(&msg,
                                         sizeof(beatled_message_hello_response_t) + 1);
    REQUIRE(result == 1);
  }

  SECTION("PROGRAM with correct size passes") {
    beatled_message_program_t msg = {.base = {.type =
                                                  BEATLED_MESSAGE_PROGRAM}};
    int result =
        validate_server_message(&msg, sizeof(beatled_message_program_t));
    REQUIRE(result == 0);
  }

  SECTION("TEMPO_RESPONSE with correct size passes") {
    beatled_message_tempo_response_t msg = {.base = {.type =
                                                         BEATLED_MESSAGE_TEMPO_RESPONSE}};
    int result =
        validate_server_message(&msg, sizeof(beatled_message_tempo_response_t));
    REQUIRE(result == 0);
  }

  SECTION("TIME_RESPONSE with correct size passes") {
    beatled_message_time_response_t msg = {.base = {.type =
                                                        BEATLED_MESSAGE_TIME_RESPONSE}};
    int result =
        validate_server_message(&msg, sizeof(beatled_message_time_response_t));
    REQUIRE(result == 0);
  }

  SECTION("NEXT_BEAT with correct size passes") {
    beatled_message_next_beat_t msg = {.base = {.type =
                                                    BEATLED_MESSAGE_NEXT_BEAT}};
    int result =
        validate_server_message(&msg, sizeof(beatled_message_next_beat_t));
    REQUIRE(result == 0);
  }

  SECTION("ERROR with correct size passes") {
    beatled_message_error_t msg = {.base = {.type = BEATLED_MESSAGE_ERROR}};
    int result =
        validate_server_message(&msg, sizeof(beatled_message_error_t));
    REQUIRE(result == 0);
  }

  SECTION("Unknown message type is rejected") {
    beatled_message_t msg = {.type = 0xFF};
    int result = validate_server_message(&msg, 10);
    REQUIRE(result == 1);
  }
}

TEST_CASE("Event handling", "[command]") {
  event_queue_init();

  SECTION("Error event returns 1") {
    event_t event = {
        .event_type = event_error,
        .time = 0,
        .data_length = 0,
        .data = NULL,
    };
    int result = handle_event(&event);
    REQUIRE(result == 1);
  }

  SECTION("Unknown event type returns 1") {
    event_t event = {
        .event_type = (event_type_t)99,
        .time = 0,
        .data_length = 0,
        .data = NULL,
    };
    int result = handle_event(&event);
    REQUIRE(result == 1);
  }

  SECTION("Server message with NULL data returns 1") {
    event_t event = {
        .event_type = event_server_message,
        .time = 1000,
        .data_length = 0,
        .data = NULL,
    };
    int result = handle_event(&event);
    REQUIRE(result == 1);
  }

  SECTION("handle_event frees event data") {
    // Allocate data that handle_event should free
    void *data = malloc(sizeof(beatled_message_error_t));
    REQUIRE(data != nullptr);
    memset(data, 0, sizeof(beatled_message_error_t));
    beatled_message_error_t *msg = (beatled_message_error_t *)data;
    msg->base.type = BEATLED_MESSAGE_ERROR;
    msg->error_code = 42;

    event_t event = {
        .event_type = event_server_message,
        .time = 1000,
        .data_length = sizeof(beatled_message_error_t),
        .data = data,
    };
    // handle_event will free event.data internally
    int result = handle_event(&event);
    REQUIRE(result == 0);
    // data is freed — we just verify no crash occurred
  }
}
