#include "timing.h"

#include <unistd.h>
#include <fcntl.h>
//#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool timing_read(const char *path, timing_t *t) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return false;

    uint64_t m;
    uint32_t h;
    uint8_t d;

    if (read(fd, &m, sizeof(m)) != sizeof(m)) { close(fd); return false; }
    if (read(fd, &h, sizeof(h)) != sizeof(h)) { close(fd); return false; }
    if (read(fd, &d, sizeof(d)) != sizeof(d)) { close(fd); return false; }

    close(fd);

    // Convert from big-endian to host byte order
    t->minutes = be64toh(m);
    t->hours = be32toh(h);
    t->daysofweek = d;
    return true;
}

bool timing_write(const char *path, const timing_t *t) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    uint64_t m = htobe64(t->minutes);
    uint32_t h = htobe32(t->hours);
    uint8_t d = t->daysofweek;

    if (write(fd, &m, sizeof(m)) != sizeof(m)) { close(fd); return false; }
    if (write(fd, &h, sizeof(h)) != sizeof(h)) { close(fd); return false; }
    if (write(fd, &d, sizeof(d)) != sizeof(d)) { close(fd); return false; }

    close(fd);
    return true;
}

// ! Les commentaires sont à enlever plus tard
bool timing_match_now(const timing_t *t) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    if (t->minutes != all_minutes &&
        !(t->minutes & (1ULL << tm_now->tm_min))) // 1ULL c'est 1 sous forme unsigned long long pr être sûr 
        // que le décalage de bit fonctionne bien sur 64 bits
        return false;

    if (t->hours != all_hours &&
        !(t->hours & (1U << tm_now->tm_hour))) // 1U c'est 1 sous forme unsigned int pr être sûr 
        // que le décalage de bit fonctionne bien sur 32 bits
        return false;

    if (t->daysofweek != all_days &&
        !(t->daysofweek & (1U << tm_now->tm_wday)))
        return false;

    return true;
}

char *timing_show(const char *path) {
    timing_t t;
    if (!timing_read(path, &t)) {
        fprintf(stderr, "Failed to read timing from %s\n", path);
        return NULL;
    }

    char *output = malloc(1024);
    if (!output) return NULL;
    output[0] = '\0';

    // Minutes (1ère version de l'affichage)
    strncat(output, "Minutes: ", 1024 - strlen(output) - 1);
    for (int i = 0; i < minutes_count; i++) {
        if (t.minutes & (1ULL << i)) {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%d ", i);
            strncat(output, tmp, sizeof(output) - strlen(output) - 1);
        }
    }

    // Hours
    strncat(output, "| Hours: ", 1024 - strlen(output) - 1);
    for (int i = 0; i < hours_count; i++) {
        if (t.hours & (1U << i)) {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%d ", i);
            strncat(output, tmp, sizeof(output) - strlen(output) - 1);
        }
    }

    // Days of Week
    strncat(output, "| Days of Week: ", 1024 - strlen(output) - 1);
    const char *day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; i < days_count; i++) { 
        if (t.daysofweek & (1U << i)) {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%s ", day_names[i]);
            strncat(output, tmp, sizeof(output) - strlen(output) - 1);
        }
    }

    char *result = strdup(output);
    if (!result) {
        fprintf(stderr, "Memory allocation failed in timing_show()\n");
        return NULL;
    }

    return result;
}