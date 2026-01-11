#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "types/timing.h"

/**
 * @brief Check if mask equals "all bits set".
 */
static bool mask_is_full(uint64_t mask, int count)
{
    uint64_t full;

    if (count >= 64)
        full = UINT64_MAX;
    else
        full = (1ULL << count) - 1;

    return (mask & full) == full;
}

/**
 * @brief Convert a bitmask into a comma-separated list, "*" or "-".
 * @return Newly allocated string (caller must free).
 */
static char *mask_to_list(uint64_t mask, int count)
{
    if (mask == 0)
        return strdup("-");

    if (mask_is_full(mask, count))
        return strdup("*");

    char buffer[256] = {0};
    for (int i = 0; i < count; i++)
    {
        if (mask & (1ULL << i))
        {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%d,", i);
            strcat(buffer, tmp);
        }
    }

    // remove trailing comma
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == ',')
        buffer[len - 1] = '\0';

    return strdup(buffer);
}

char *timing_to_string(const timing_t *t)
{
    if (!t)
        return strdup("- - -");

    char *min = mask_to_list(t->minutes, MINUTES_COUNT);
    char *hrs = mask_to_list(t->hours, HOURS_COUNT);
    char *day = mask_to_list(t->daysofweek, DAYS_COUNT);

    size_t size = strlen(min) + strlen(hrs) + strlen(day) + 3; // spaces + null terminator
    char *out = malloc(size);

    snprintf(out, size, "%s %s %s", min, hrs, day);

    free(min);
    free(hrs);
    free(day);
  
    return out;
}

bool timing_match_at(const timing_t *t, time_t now)
{
    struct tm tm_now;
    time_t minute_now = now - (now % 60);
    localtime_r(&minute_now, &tm_now);

    if(t -> minutes == 0 && t->hours == 0 && t->daysofweek == 0){
        return false; // timing abstract
    }
    // minutes
    if (t->minutes != 0 && !mask_is_full(t->minutes, 60)) {
        if (!(t->minutes & (1ULL << tm_now.tm_min))) return false;
    }

    // hours
    if (t->hours != 0 && !mask_is_full(t->hours, 24)) {
        if (!(t->hours & (1U << tm_now.tm_hour))) return false;
    }

    // day of the week
    if (t->daysofweek != 0 && !mask_is_full(t->daysofweek, 7)) {
        
        int wday = tm_now.tm_wday == 0 ? 6 : tm_now.tm_wday - 1; 
        if (!(t->daysofweek & (1U << wday))) return false;
    }

    return true;
}


timing_t* timing_create(const char *data, ssize_t size)
{
    // A timing is exactly 13 bytes (8 + 4 + 1)
    ssize_t NEED = sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t);

    if (data == NULL || size < NEED) {
        dprintf(STDERR_FILENO, "timing_show: buffer too small or NULL\n");
        return NULL;
    }

    timing_t* timing = malloc(sizeof(timing_t));
    if (timing == NULL) {
        perror("malloc");
        return NULL;
    }
    ssize_t off = 0;

    uint64_t m_be;
    memcpy(&m_be, data + off, sizeof(uint64_t));
    timing->minutes = be64toh(m_be);
    off += sizeof(uint64_t);

    uint32_t h_be;
    memcpy(&h_be, data + off, sizeof(uint32_t));
    timing->hours = be32toh(h_be);
    off += sizeof(uint32_t);

    uint8_t d;
    memcpy(&d, data + off, sizeof(uint8_t));
    timing->daysofweek = d;

    return timing;
}

void timing_show(const timing_t *t)
{
    char *txt = timing_to_string(t);
    dprintf(STDOUT_FILENO, "%s ", txt);
    free(txt);
}

timing_t* timing_copy(const timing_t* src){
    if (!src) return NULL;

    timing_t* t = malloc(sizeof(timing_t));
    if (!t) return NULL;

    memcpy(t, src, sizeof(timing_t));
    return t;
}

timing_t* timing_create_from_strings(const char *minutes_str, const char *hours_str, const char *days_of_week_str){

    timing_t* t = malloc(sizeof(timing_t));
    
    if (!t) {
        perror("malloc");
        return NULL;
    }

    // Minutes
    if (minutes_str == NULL || strcmp(minutes_str, "*") == 0) {
        t->minutes = ALL_MINUTES;
    }
    else if (strcmp(minutes_str, "-") == 0) {
        t->minutes = 0;
    }
    else {
        t->minutes = 0;
        char *token;
        char *str_copy = strdup(minutes_str);
        char *rest = str_copy;

        while ((token = strtok_r(rest, ",", &rest))) {
            int minute = atoi(token);

            if (minute < 0 || minute >= MINUTES_COUNT) {
                dprintf(STDERR_FILENO, "Invalid minute value: %s\n", token);
                free(str_copy);
                free(t);
                return NULL;
            }
            t->minutes |= (1ULL << minute);
        }
        free(str_copy);
    }

    // Hours
    if (hours_str == NULL || strcmp(hours_str, "*") == 0) {
        t->hours = ALL_HOURS;
    }
    else if (strcmp(hours_str, "-") == 0) {
        t->hours = 0;
    }
    else {
        t->hours = 0;
        char *token;
        char *str_copy = strdup(hours_str);
        char *rest = str_copy;

        while ((token = strtok_r(rest, ",", &rest))) {
            int hour = atoi(token);

            if (hour < 0 || hour >= HOURS_COUNT) {

                dprintf(STDERR_FILENO, "Invalid hour value: %s\n", token);
                free(str_copy);
                free(t);
                return NULL;
            }
            t->hours |= (1U << hour);
        }
        free(str_copy);
    }

    // Days of the week
    if (days_of_week_str == NULL || strcmp(days_of_week_str, "*") == 0) {
        t->daysofweek = ALL_DAYS;
    }
    else if (strcmp(days_of_week_str, "-") == 0) {
        t->daysofweek = 0;
    }
    else {
        t->daysofweek = 0;
        char *token;
        char *str_copy = strdup(days_of_week_str);
        char *rest = str_copy;

        while ((token = strtok_r(rest, ",", &rest))) {

            int day = atoi(token);
            if (day < 0 || day >= DAYS_COUNT) {

                dprintf(STDERR_FILENO, "Invalid day value: %s\n", token);
                free(str_copy);
                free(t);
                return NULL;
            }
            t->daysofweek |= (1U << day);
        }
        free(str_copy);
    }
    return t;
}

void timing_free(timing_t* t){
    if(t){
        free(t);
    }
}

void timing_set_abstract(timing_t* t){
    if(t){
        t->minutes = 0;
        t->hours = 0;
        t->daysofweek = 0;
    }
}