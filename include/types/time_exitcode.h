#ifndef TIME_EXITCODE_H
#define TIME_EXITCODE_H

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

/**
 * @brief Represents a single execution record (timestamp + exit code).
 * Serialization format: TIME <uint64_t> | EXITCODE <int32_t>
 */
typedef struct __attribute__((packed)) {
    int64_t time;     // timestamp of execution
    uint16_t exitcode;  // Exit status of the task
} time_exitcode_t;

/**
 * @brief Append a new (time, exitcode) record to the log file.
 * @param path Path to "times-exitcodes" file.
 * @param record Pointer to the record to append.
 * @return true on success, false on failure.
 */
bool time_exitcode_append(const char *path, const time_exitcode_t *record);

/**
 * @brief Read and display all previous records from the file.
 * @param path Path to "times-exitcodes" file.
 * @return true on success, false on failure.
 */
char *time_exitcode_show(const char *data, ssize_t size);

#endif