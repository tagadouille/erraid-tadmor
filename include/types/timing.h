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
 * @brief Turns a timing into a printable string.
 * @return Newly allocated char* ; caller MUST free().
 */
char *timing_to_string(const timing_t *t);

/**
 * @brief Check if the current time matches the timing_t structure.
 * @param t Pointer to the timing_t structure to check against.
 * @return true if the current time matches, false otherwise.
 */
bool timing_match_at(const timing_t *t, time_t when);

/**
 * @brief Read and interpret a binary timing file into a readable string.
 * @param path Path to the timing file.
 * @return the timing_t structure if success, NULL otherwise
 */
timing_t* timing_create(const char *path, ssize_t size);

/**
 * @brief copy a timing struct 
 * @param a pointer to the struct to copy
 * @return a pointer to the copy, NULL if failure
 */
timing_t* timing_copy(const timing_t* src);

/**
 * @brief Display the timing information.
 * @param t Pointer to the timing_t structure to display.
 */
void timing_show(const timing_t *t);

/** 
 * @brief Create a timing_t structure from string representations.
 * @param minutes_str string representing minutes
 * @param hours_str string representing hours
 * @param days_of_week_str string representing days of the week
 * @return the timing_t structure if success, NULL otherwise
 */
timing_t* timing_create_from_strings(const char *minutes_str, const char *hours_str, const char *days_of_week_str);

/**
 * @brief Free the memory allocated for a timing_t structure.
 * @param t Pointer to timing_t to free
 */
void timing_free(timing_t* t);

/** 
 * @brief Set the timing_t structure to represent an abstract task (always true).
 * @param t Pointer to the timing_t structure to modify.
 */
void timing_set_abstract(timing_t* t);

#endif