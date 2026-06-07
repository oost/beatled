#ifndef BEATLED_CONFIG_PORT_NAME_H
#define BEATLED_CONFIG_PORT_NAME_H

// String identifying which port is running. Compile-time constant — picked
// up by the boot banner, HELLO_REQUEST's port_name field, and any other
// place that wants to surface the running flavour to the server / logs.
//
// Keep the values short (<16 bytes including the NUL terminator) to fit
// the wire-protocol field in beatled_message_hello_request_t.

#if defined(PICO_PORT) && defined(FREERTOS_PORT)
#define BEATLED_PORT_NAME "pico-freertos"
#elif defined(PICO_PORT)
#define BEATLED_PORT_NAME "pico"
#elif defined(POSIX_PORT) && defined(FREERTOS_PORT)
#define BEATLED_PORT_NAME "posix-freertos"
#elif defined(POSIX_PORT)
#define BEATLED_PORT_NAME "posix"
#elif defined(ESP32_PORT)
#define BEATLED_PORT_NAME "esp32"
#else
#define BEATLED_PORT_NAME "unknown"
#endif

#endif // BEATLED_CONFIG_PORT_NAME_H
