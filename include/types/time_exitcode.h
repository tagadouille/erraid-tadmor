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

typedef struct{
    uint32_t nbruns;
    time_exitcode_t* all_timecode;
} time_array_t;

/**
 * @brief Append a new (time, exitcode) record to the log file.
 * @param path Path to "times-exitcodes" file.
 * @param record Pointer to the record to append.
 * @return true on success, false on failure.
 */
bool time_exitcode_append(const char *path, const time_exitcode_t *record);

/**
 * @brief Parse the buffer that contains the content of the times_exitcodes
 * file and return a pointer to the array of times_exitcodes
 * @param data the buffer
 * @param size the size of the buffer
 * @return a pointer to the array of times_exitcodes, NULL pointer on failure
 */
time_array_t *time_exitcode_parse(const char *data, ssize_t size);

/**
 * @brief display the time_exitcode_t structure
 * @param te a pointer to a time_exitcode_t structure
 */
void time_exitcode_show(const time_exitcode_t* te);

/**
 * @brief display all the time_exitcode_t structure of the array
 * @param te_arr a pointer to a time_array_t structure
 */
void all_time_show(time_array_t* te_arr);

/**
 * @brief free the memory allocated for a time_array_t structure
 * @param te_arr a pointer to a time_array_t structure
 */
void time_array_free(time_array_t* te_arr)

#endif