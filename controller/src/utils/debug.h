#ifndef UTILS_DEBUG_H
#define UTILS_DEBUG_H

// From https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c

#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

// #define debug_print(fmt, ...) \
            // do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

#define debug_print_long(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#endif // UTILS_DEBUG_H

