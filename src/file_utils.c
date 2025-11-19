#define _GNU_SOURCE
#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

char *file_read_all(const char *path, ssize_t *size_out)
{
    if (!path || !size_out)
        return NULL;

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return NULL;

    // Determine file size
    ssize_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        close(fd);
        return NULL;
    }

    lseek(fd, 0, SEEK_SET);

    char *buf = malloc(size);
    if (!buf) {
        close(fd);
        return NULL;
    }

    ssize_t read_bytes = read(fd, buf, size);
    close(fd);

    if (read_bytes != size) {
        free(buf);
        return NULL;
    }

    *size_out = size;
    return buf;
}

char *path_join(const char *base, const char *child)
{
    if (!base || !child)
        return NULL;

    size_t len = strlen(base) + 1 + strlen(child) + 1;
    char *out = malloc(len);
    if (!out)
        return NULL;

    snprintf(out, len, "%s/%s", base, child);
    return out;
}