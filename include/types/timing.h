#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <stdbool.h>

#define minutes_count 60
#define hours_count 24
#define days_count 7

#define all_minutes ((1ULL << minutes_count) - 1)
#define all_hours ((1ULL << hours_count) - 1)
#define all_days ((1ULL << days_count) - 1)

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
 * @brief Print the timing_t structure.
 * @param t Pointer to the timing_t structure to print.
 */
void timing_print(const timing_t *t);

#endif