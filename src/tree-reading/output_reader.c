#define _GNU_SOURCE

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
    size_t capacity = 0;
    size_t size = 0;
    int result = 0;
    int fd = -1;

    // Buffer initialization
    capacity = 4096;
    buffer = malloc(capacity);
    if (!buffer) {
        perror("malloc initial buffer");
        return -1;
    }

    // Determine if the file exist : 
    if(access(path, F_OK) != 0) {
        free(buffer);
        dprintf(STDERR_FILENO, "The output file does not exist\n");
        return -2;
    }

    // Open the file
    fd = open(path, O_RDONLY);
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

        // Enlarge the buffer if necessary
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

    // Free the old curr_output if it exists
    if (curr_output != NULL) {
        string_free(curr_output);
        curr_output = NULL;
    }

    if (result == 0) {

        curr_output = string_create(buffer, size);
        if (!curr_output) {
            dprintf(STDERR_FILENO, "output_reader: string_create failed\n");
            result = -1;
        }
        if(size == 0){
            result = -2; // Indicate empty output
        }
    }

    free(buffer);
    if (fd >= 0 && close(fd) != 0) {
        perror("close");
        result = -1;
    }
    
    return result;
}