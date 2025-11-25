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

    size_t size = strlen(min) + strlen(hrs) + strlen(day) + 8;
    char *out = malloc(size);

    snprintf(out, size, "%s %s %s", min, hrs, day);

    free(min);
    free(hrs);
    free(day);
  
    return out;
}

// ! Les commentaires sont à enlever plus tard
bool timing_match_now(const timing_t *t)
{
    if (!t)
        return false;

    // Get current time
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    // Check minutes mask
    if (t->minutes == 0)
        return false;  // Aucun bit = jamais valable

    if (!mask_is_full(t->minutes, MINUTES_COUNT) &&
        !(t->minutes & (1ULL << tm_now->tm_min)))
        return false; // 1ULL c'est 1 sous forme unsigned long long pr être sûr
                    // que le décalage de bit fonctionne bien sur 64 bits
    
    // Check hours mask
    if (t->hours == 0)
        return false;  // Aucun bit = jamais valable

    if (!mask_is_full(t->hours, HOURS_COUNT) &&
        !(t->hours & (1U << tm_now->tm_hour)))
        return false; // 1U c'est 1 sous forme unsigned int pr être sûr
                    // que le décalage de bit fonctionne bien sur 32 bits

    // Check days of week mask
    if (t->daysofweek == 0)
        return false;  // Aucun bit = jamais valable

    if (!mask_is_full(t->daysofweek, DAYS_COUNT) &&
        !(t->daysofweek & (1U << tm_now->tm_wday)))
        return false;
    
    return true;
}

//TODO Refaire ça
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
    dprintf(STDOUT_FILENO, "%s", txt);
    free(txt);
}

bool timing_should_run(const timing_t *t)
{
    if (!t)
        return false;
    return timing_match_now(t);
}