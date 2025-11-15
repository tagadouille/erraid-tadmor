#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "types/timing.h"

// ! Les commentaires sont à enlever plus tard
bool timing_match_now(const timing_t *t)
{
    // Get current time
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    // Check minutes mask
    if (t->minutes != ALL_MINUTES &&
        !(t->minutes & (1ULL << tm_now->tm_min))) // 1ULL c'est 1 sous forme unsigned long long pr être sûr
    // que le décalage de bit fonctionne bien sur 64 bits
    {
        return false;
    }

    // Check hours mask
    if (t->hours != ALL_HOURS &&
        !(t->hours & (1U << tm_now->tm_hour))) // 1U c'est 1 sous forme unsigned int pr être sûr
    // que le décalage de bit fonctionne bien sur 32 bits
    {
        return false;
    }
    // Check days of week mask
    if (t->daysofweek != ALL_DAYS &&
        !(t->daysofweek & (1U << tm_now->tm_wday)))
    {
        return false;
    }
    return true;
}

char *timing_show(const char *data, ssize_t size)
{
    // A timing is exactly 13 bytes (8 + 4 + 1)
    const ssize_t TIMING_BIN_SIZE = 8 + 4 + 1;

    if (data == NULL || size < TIMING_BIN_SIZE) {
        dprintf(STDERR_FILENO, "timing_show: buffer too small or NULL\n");
        return NULL;
    }

    timing_t t;
    ssize_t offset = 0;

    uint64_t m_be;
    uint32_t h_be;
    uint8_t  d;

    // ---- minutes ----
    memcpy(&m_be, data + offset, sizeof(uint64_t));
    t.minutes = be64toh(m_be);
    offset += sizeof(uint64_t);

    // ---- hours ----
    memcpy(&h_be, data + offset, sizeof(uint32_t));
    t.hours = be32toh(h_be);
    offset += sizeof(uint32_t);

    // ---- days ----
    memcpy(&d, data + offset, sizeof(uint8_t));
    t.daysofweek = d;

    // ---- allocate output ----
    char *output = malloc(2048);
    if (!output)
        return NULL;

    output[0] = '\0';

    // ---- Minutes ----
    strncat(output, "Minutes: ", 2048 - strlen(output) - 1);
    for (int i = 0; i < MINUTES_COUNT; i++) {
        if (t.minutes & (1ULL << i)) {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%d ", i);
            strncat(output, tmp, 2048 - strlen(output) - 1);
        }
    }

    // ---- Hours ----
    strncat(output, "| Hours: ", 2048 - strlen(output) - 1);
    for (int i = 0; i < HOURS_COUNT; i++) {
        if (t.hours & (1U << i)) {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%d ", i);
            strncat(output, tmp, 2048 - strlen(output) - 1);
        }
    }

    // ---- Days ----
    strncat(output, "| Days of Week: ", 2048 - strlen(output) - 1);
    const char *day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; i < DAYS_COUNT; i++) {
        if (t.daysofweek & (1U << i)) {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%s ", day_names[i]);
            strncat(output, tmp, 2048 - strlen(output) - 1);
        }
    }

    char *result = malloc(strlen(output) + 1);
    if (!result) {
        free(output);
        return NULL;
    }

    strcpy(result, output);
    free(output);
    return result;
}