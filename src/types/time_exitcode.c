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

bool time_exitcode_append(const char *path, const time_exitcode_t *record)
{
    // Open file in append mode, create if it doesn't exist
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644); // 0644 : rw-r--r--
    if (fd < 0)
        return false;

    // Convert to big-endian before writing (on inverse l'ordre des octets)
    uint64_t t = htobe64(record->time);
    uint32_t c = htobe32(record->exitcode);

    // Write timestamp and exitcode
    if (write(fd, &t, sizeof(t)) != sizeof(t))
    {
        close(fd);
        return false;
    }
    if (write(fd, &c, sizeof(c)) != sizeof(c))
    {
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

char *time_exitcode_show(const char *data, ssize_t size){
    
    const ssize_t REC_SIZE = sizeof(uint64_t) + sizeof(uint32_t); 

    // Validation du buffer
    if (data == NULL || size <= 0) {
        dprintf(STDERR_FILENO, "Invalid buffer size=%ld (not a multiple of 12)\n", size);
        return NULL;
    }

    if (size % REC_SIZE != 0)
    {
        dprintf(STDERR_FILENO, "Invalid buffer size=%ld (not a multiple of 12)\n", size);
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
        uint64_t t_be;
        memcpy(&t_be, data + offset, sizeof(uint64_t));
        time_t ts = (time_t)be64toh(t_be);
        offset += sizeof(uint64_t);

        // ---- exitcode ----
        uint32_t c_be;
        memcpy(&c_be, data + offset, sizeof(uint32_t));
        unsigned int exitcode = be32toh(c_be);
        offset += sizeof(uint32_t);

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

    return output;   // caller must free()
}
