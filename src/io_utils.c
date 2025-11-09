#include "io_utils.h"
#include <unistd.h>     
#include <errno.h>     
 
int read_all(int fd, void* buf, size_t count) {
    size_t bytes_read = 0;
    while (bytes_read < count) {
        ssize_t ret = read(fd, (char*)buf + bytes_read, count - bytes_read);
        
        if (ret == 0) {
            return -1; // EOF before reading 'count' bytes
        }
        if (ret == -1) {
            if (errno == EINTR) {
                continue; // interrupted, retry
            }
            return -1; // Other error
        }
        bytes_read += ret;
    }
    return 0;
}

int write_all(int fd, const void* buf, size_t count) {
    size_t bytes_written = 0;
    while (bytes_written < count) {
        ssize_t ret = write(fd, (const char*)buf + bytes_written, count - bytes_written);
        if (ret == -1) {
            if (errno == EINTR) {
                continue; // interrupted, retry
            }
            return -1; // Other error
        }
        bytes_written += ret;
    }
    return 0;
}