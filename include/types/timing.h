#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MINUTES_COUNT 60
#define HOURS_COUNT 24
#define DAYS_COUNT 7

#define ALL_MINUTES ((1ULL << MINUTES_COUNT) - 1) // Décalage du nombre d'octet de MINUTES
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
 * @brief Check if the current time matches the timing_t structure.
 * @param t Pointer to the timing_t structure to check against.
 * @return true if the current time matches, false otherwise.
 */
bool timing_match_now(const timing_t *t);

/**
 * @brief Read and interpret a binary timing file into a readable string.
 * @param path Path to the timing file.
 * @return the timing_t structure if success, NULL otherwise
 */
timing_t* timing_create(const char *path, ssize_t size);

/**
 * @brief Display the timing information.
 * @param t Pointer to the timing_t structure to display.
 */
void timing_show(const timing_t *t);

/**
 * @brief Return true if the task should run now
 */
bool timing_should_run(const timing_t *t);

#endif