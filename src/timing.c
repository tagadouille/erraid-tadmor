#include "timing.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
//#include <endian.h>
#include <string.h> 

/**
 * @brief Read exactly 'count' bytes from 'fd' into 'buf'.
 * Handle partial reads and interruptions.
 * @return 0 on success, -1 on error or premature EOF
 */
static int read_all(int fd, void* buf, size_t count) {
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

/**
 * @brief Write exactly 'count' bytes from 'buf' to 'fd'.
 * Handle partial writes and interruptions.
 * @return 0 on success, -1 on error.
 */
static int write_all(int fd, const void* buf, size_t count) {
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


timing_t* timing_new_empty(void) {
    timing_t* t = (timing_t*)malloc(sizeof(timing_t));
    if (t == NULL) return NULL;
    
    // Initialisation à zéro (tous les bits à 0 = jamais)
    t->minutes = 0;
    t->hours = 0;
    t->daysofweek = 0;
    
    return t;
}

void free_timing(timing_t* t) {
    free(t);
}

timing_t* read_timing(int fd) {
    timing_t* t = timing_new_empty();
    if (t == NULL) return NULL;

    uint64_t network_minutes;
    uint32_t network_hours;
    uint8_t network_daysofweek;

    // Read MINUTES (uint64)
    if (read_all(fd, &network_minutes, sizeof(network_minutes)) == -1) {
        free_timing(t);
        return NULL;
    }
    t->minutes = be64toh(network_minutes);

    // Read HOURS (uint32)
    if (read_all(fd, &network_hours, sizeof(network_hours)) == -1) {
        free_timing(t);
        return NULL;
    }
    t->hours = be32toh(network_hours);

    // Read DAYSOFWEEK (uint8)
    // No conversion necessary for uint8
    if (read_all(fd, &network_daysofweek, sizeof(network_daysofweek)) == -1) {
        free_timing(t);
        return NULL;
    }
    t->daysofweek = network_daysofweek;

    return t;
}


int write_timing(int fd, const timing_t* t) {
    if (t == NULL) return -1;

    // Write MINUTES (uint64)
    uint64_t network_minutes = htobe64(t->minutes);
    if (write_all(fd, &network_minutes, sizeof(network_minutes)) == -1) {
        return -1;
    }

    // Write HOURS (uint32)
    uint32_t network_hours = htobe32(t->hours);
    if (write_all(fd, &network_hours, sizeof(network_hours)) == -1) {
        return -1;
    }

    // Write DAYSOFWEEK (uint8)
    // No conversion necessary for uint8
    if (write_all(fd, &(t->daysofweek), sizeof(t->daysofweek)) == -1) {
        return -1;
    }

    return 0;
}