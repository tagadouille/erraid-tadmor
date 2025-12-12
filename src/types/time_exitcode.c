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

char *time_exitcode_show(const char *data, ssize_t size)
{

    const ssize_t REC_SIZE = sizeof(int64_t) + sizeof(uint16_t);
    size = (size / 10) * 10;
    // Validation du buffer
    if (data == NULL || size <= 0)
    {
        dprintf(STDERR_FILENO, "Invalid buffer size=%ld (not a multiple of %ld)\n", size, REC_SIZE);
        return NULL;
    }

    if (size % REC_SIZE != 0)
    {
        dprintf(STDERR_FILENO, "Invalid buffer size=%ld (not a multiple of %ld)\n", size, REC_SIZE);
        return NULL;
    }

    // Output final buffer (2 KB is enough)
    char *output = malloc(2048);
    if (!output)
        return NULL;
    output[0] = '\0';

    size_t offset = 0;
    char line[128];

    while (offset + REC_SIZE <= (size_t)size)
    {
        // ---- timestamp ----
        int64_t t_be;
        memcpy(&t_be, data + offset, sizeof(int64_t));
        time_t ts = (time_t)be64toh(t_be);
        offset += sizeof(int64_t);

        // ---- exitcode ----
        uint16_t c_be;
        memcpy(&c_be, data + offset, sizeof(uint16_t));
        unsigned int exitcode = be16toh(c_be);
        offset += sizeof(uint16_t);

        // ---- format timestamp ----
        char time_str[32];
        struct tm *tm_info = localtime(&ts);

        if (tm_info)
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        else
            strcpy(time_str, "Invalid time");

        // ---- format line ----
        snprintf(line, sizeof(line), "%s %d\n", time_str, exitcode);

        // ---- append to output ----
        if (strlen(output) + strlen(line) < 2047)
            strcat(output, line);
    }

    // If no lines
    if (strlen(output) == 0)
        strcpy(output, "(No previous executions)\n");

    return output; // caller must free()
}