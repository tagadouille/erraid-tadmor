#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "types/timing.h"

/**
 * @brief Append "i " for each bit set in mask into buffer.
 */
static void append_int_list(char *out, size_t max, uint64_t mask, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (mask & (1ULL << i))
        {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%d ", i);
            strncat(out, tmp, max - strlen(out) - 1);
        }
    }
}

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

//TODO Refaire ça
timing_t* timing_create(const char *data, ssize_t size)
{
    // A timing is exactly 13 bytes (8 + 4 + 1)
    const ssize_t NEED = sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t);

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
    if (!t) {
        dprintf(STDERR_FILENO, "timing_show: NULL timing pointer\n");
        return;
    }

    // ---- allocate output ----
    char out[2048];
    out[0] = '\0';

    //TODO Modify it to be less readable but more efficient
    strncat(out, "Minutes: ", sizeof(out) - strlen(out) - 1);
    append_int_list(out, sizeof(out), t->minutes, MINUTES_COUNT);

    strncat(out, "| Hours: ", sizeof(out) - strlen(out) - 1);
    append_int_list(out, sizeof(out), t->hours, HOURS_COUNT);

    static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    strncat(out, "| Days: ", sizeof(out) - strlen(out) - 1);

    for (int i = 0; i < DAYS_COUNT; i++)
    {
        if (t->daysofweek & (1U << i))
        {
            strncat(out, days[i], sizeof(out) - strlen(out) - 1);
            strncat(out, " ", sizeof(out) - strlen(out) - 1);
        }
    }

    dprintf(STDOUT_FILENO, "%s\n", out);
}

bool timing_should_run(const timing_t *t)
{
    if (!t)
        return false;

    return timing_match_now(t);
}