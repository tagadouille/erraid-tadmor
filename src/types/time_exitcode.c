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
    int32_t c = htobe32(record->exitcode);

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

char *time_exitcode_show(const char *path)
{
    // Open file for reading
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return NULL;
    }

    printf("=== Past executions ===\n");
    // Allocate output buffer for formattted lines
    char *output = malloc(1024);
    if (!output)
        return NULL;
    output[0] = '\0';

    time_exitcode_t record;
    uint64_t t;
    int32_t c;
    char line[128];
    // Read records until EOF
    while (read(fd, &t, sizeof(t)) == sizeof(t) &&
           read(fd, &c, sizeof(c)) == sizeof(c))
    {
        // Convert from big-endian to host order
        record.time = be64toh(t);
        record.exitcode = be32toh(c);

        // Format the record for display
        time_t ts = (time_t)record.time;
        struct tm *tm_info = localtime(&ts);

        char time_str[64];
        if (tm_info)
        {
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        }
        else
        {
            snprintf(time_str, sizeof(time_str), "Invalid time");
        }
        // Format: "YYYY-MM-DD HH:MM:SS <exitcode>"
        snprintf(line, sizeof(line), "%s %d\n", time_str, record.exitcode);

        // Append line to output buffer (simple bound check)
        if (strlen(output) + strlen(line) < 1023)
            strcat(output, line);
    }

    close(fd);

    // If no record was found
    if (strlen(output) == 0)
        strcpy(output, "(No previous executions)\n");

    return output;
}

// ! Cette fonction sera supprimée dans la version finale du code.
void time_exitcode_print(const time_exitcode_t *record)
{
    time_t ts = (time_t)record->time;
    struct tm *tm_info = localtime(&ts);

    char time_str[64];
    if (tm_info)
    {
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    }
    else
    {
        snprintf(time_str, sizeof(time_str), "Invalid time");
    }

    printf("[%s] → Exit code: %d\n", time_str, record->exitcode);
}