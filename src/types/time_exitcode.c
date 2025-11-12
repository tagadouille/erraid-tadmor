#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include "types/time_exitcode.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <endian.h>

bool time_exitcode_append(const char *path, const time_exitcode_t *record) {
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644); // 0644 : rw-r--r--
    if (fd < 0) return false;

    // Convert to big-endian before writing (on inverse l'ordre des octets)
    uint64_t t = htobe64(record->time); 
    int32_t  c = htobe32(record->exitcode);

    // Write timestamp and exitcode
    if (write(fd, &t, sizeof(t)) != sizeof(t)) { close(fd); return false; }
    if (write(fd, &c, sizeof(c)) != sizeof(c)) { close(fd); return false; }

    close(fd);
    return true;
}

char *time_exitcode_show(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    printf("=== Past executions ===\n");
    char *output = malloc(1024);
    if (!output) return NULL;
    output[0] = '\0';

    time_exitcode_t record;
    uint64_t t;
    int32_t c;
    char line[128];

    while (read(fd, &t, sizeof(t)) == sizeof(t) &&
           read(fd, &c, sizeof(c)) == sizeof(c)) {
        record.time = be64toh(t);
        record.exitcode = be32toh(c);

        time_t ts = (time_t)record.time;
        struct tm *tm_info = localtime(&ts);

        char time_str[64];
        if (tm_info) {
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            snprintf(time_str, sizeof(time_str), "Invalid time");
        }

        snprintf(line, sizeof(line), "[%s] → Exit code: %d\n", time_str, record.exitcode);

        if (strlen(output) + strlen(line) < 1023)
            strcat(output, line);
    }

    close(fd);

    if (strlen(output) == 0)
        strcpy(output, "(No previous executions)\n");

    return output;
}