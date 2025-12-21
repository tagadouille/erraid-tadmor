#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include "tree-reading/tree_reader.h"
#include "erraids/erraid.h"

int output_reader(const char* path, bool is_stderr) {
    
    char* buffer = NULL;

    if(buffer_init(&buffer) == -1){
        return -1;
    }

    size_t capacity = BUFFER_SIZE;
    size_t size = 0;
    int result = 0;

    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        if (is_stderr)
            perror("open standard error file");
        else
            perror("open standard output file");
        free(buffer);
        return -1;
    }

    while (1) {
        ssize_t nread = read(fd, buffer + size, capacity - size);

        if (nread < 0) {
            if (errno == EINTR) continue;
            perror("read");
            result = -1;
            break;
        }
        if (nread == 0) break;

        size += (size_t)nread;

        // Enlarge the buffer if necessary :
        if (size == capacity) {

            size_t new_capacity = capacity * 2;
            char* tmp = realloc(buffer, new_capacity);
            if (!tmp) {
                perror("realloc");
                result = -1;
                break;
            }
            buffer = tmp;
            capacity = new_capacity;
        }
    }

    if (result == 0) {

        if (size == capacity) {
            char* tmp = realloc(buffer, capacity + 1);
            if (!tmp) {
                perror("realloc");
                free(buffer);
                close(fd);
                return -1;
            }
            buffer = tmp;
        }
        buffer[size] = '\0';

        curr_output = string_create(buffer, size);
    }

    free(buffer);
    if (close(fd) != 0) {
        perror("close");
        result = -1;
    }
    return result;
}