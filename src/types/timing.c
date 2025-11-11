#include "timing.h"

#include <unistd.h>
//#include <endian.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

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

void timing_print(const timing_t *t) {
    printf("Minutes: ");
    for (int i = 0; i < minutes_count; i++) {
        if (t->minutes & (1ULL << i)) {
            printf("%d ", i);
        }
    }
    printf("\nHours: ");
    for (int i = 0; i < hours_count; i++) {
        if (t->hours & (1U << i)) {
            printf("%d ", i);
        }
    }
    printf("\nDays of Week: ");
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; i < days_count; i++) {
        if (t->daysofweek & (1U << i)) {
            printf("%s ", days[i]);
        }
    }
    printf("\n");
}