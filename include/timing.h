#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Reprensent type 'timing' 
 * Describe the execution times of a task (minutes, hours, days of the week).
 * The serialization format is :
 * MINUTES <uint64> (BE) | HOURS <uint32> (BE) | DAYSOFWEEK <uint8> (BE, but just a byte)
 */
typedef struct 
{
    uint64_t minutes;   // (0-59)
    uint32_t hours;     // (0-23)
    uint8_t daysofweek; // (Sunday=0 to Saturday=6)
} timing_t;

/**
 * @brief Allocate a new timing_t and initialize it to zero values.
 * @return Pointer to a new timing_t allocated, or NULL in case of error
 */
timing_t* timing_new_empty(void);

/**
 * @brief Free the memory allocated for a timing_t.
 * @param t timing_t to free
 */
void free_timing(timing_t* t);

/**
 * @brief Read a timing_t from a file descriptor.
 * @param fd file descriptor
 * @return pointer to a new timing_t allocated, or NULL in case of error
 */
timing_t* read_timing(int fd);

/**
 * @brief Write timing_t to a file descriptor.
 * @param fd file descriptor
 * @param t timing_t to write
 * @return 0 when success, -1 in case of error
 */
int write_timing(int fd, const timing_t* t);

#endif