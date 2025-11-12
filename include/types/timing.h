#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MINUTES_COUNT 60
#define HOURS_COUNT 24
#define DAYS_COUNT 7

#define ALL_MINUTES ((1ULL << MINUTES_COUNT) - 1)
#define ALL_HOURS ((1ULL << HOURS_COUNT) - 1)
#define ALL_DAYS ((1ULL << DAYS_COUNT) - 1)

/**
 * @brief Reprensent type 'timing' 
 * Describe the execution times of a task (minutes, hours, days of the week).
 * The serialization format is :
 * MINUTES <uint64> | HOURS <uint32> | DAYSOFWEEK <uint8>
 */
typedef struct 
{
    uint64_t minutes;   // (0-59)
    uint32_t hours;     // (0-23)
    uint8_t daysofweek; // (Sunday=0 to Saturday=6)
} timing_t;

/**
 * @brief Read the timing_t structure from a file descriptor.
 * @param path Path to the file to read from.
 * @param t Pointer to the timing_t structure to fill.
 * @return true on success, false on failure.
 */
bool timing_read(const char *path, timing_t *t);

/**
 * @brief Write the timing_t structure to a file descriptor.
 * @param path Path to the file to write to.
 * @param t Pointer to the timing_t structure to write.
 * @return true on success, false on failure.
 */
bool timing_write(const char *path, const timing_t *t);

/**
 * @brief Check if the current time matches the timing_t structure.
 * @param t Pointer to the timing_t structure to check against.
 * @return true if the current time matches, false otherwise.
 */
bool timing_match_now(const timing_t *t);

/**
 * @brief Read and interpret a binary timing file into a readable string.
 * @param path Path to the timing file.
 * @return Newly allocated string (must be freed by caller), or NULL on failure.
 */
char *timing_show(const char *path);

#endif