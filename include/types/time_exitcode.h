#ifndef TIME_EXITCODE_H
#define TIME_EXITCODE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Represents a single execution record (timestamp + exit code).
 * Serialization format: TIME <uint64_t> | EXITCODE <int32_t>
 */
typedef struct 
{
    uint64_t time;     // timestamp of execution
    int32_t exitcode;  // Exit status of the task
} time_exitcode_t;


/**
 * @brief Allocates a new time_exitcode_t structure.
 * @return A pointer to a new time_exitcode_t, or NULL on error.
 */
time_exitcode_t* time_exitcode_new(void);

/**
 * @brief Frees the memory allocated for a time_exitcode_t.
 * @param record The record to free.
 */
void free_time_exitcode(time_exitcode_t* record);

/**
 * @brief Reads (deserializes) a time_exitcode_t from a file descriptor (fd).
 * @param fd The file descriptor.
 * @return A pointer to a newly allocated time_exitcode_t, or NULL on error.
 */
time_exitcode_t* read_time_exitcode(int fd);

/**
 * @brief Writes (serializes) a time_exitcode_t to a file descriptor (fd).
 * @param fd The file descriptor.
 * @param record The record to write.
 * @return 0 on success, -1 on error.
 */
int write_time_exitcode(int fd, const time_exitcode_t* record);

#endif