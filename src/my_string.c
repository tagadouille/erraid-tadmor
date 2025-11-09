#include "my_string.h"
#include <unistd.h>
// #include <endian.h>
#include <stdlib.h>
#include <errno.h>

/**
 * @brief Read exactly 'count' bytes from 'fd' to 'buf'.
 *
 * @return 0 when Succeed, -1 when Error
 */
static int read_all(int fd, void *buf, size_t count)
{
    size_t bytes_read = 0;
    while (bytes_read < count)
    {
        ssize_t ret = read(fd, (char *)buf + bytes_read, count - bytes_read);
        if (ret == 0)
        { // EOF (without having read 'count' bytes)
            return -1;
        }
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue; // Interrupted, Try again
            }
            return -1; // ERROR
        }
        bytes_read += ret;
    }
    return 0;
}

/**
 * @brief Write exactly 'count' bytes from 'buf' to 'fd'.
 *
 * @return 0 Succeed, -1 Error
 */
static int write_all(int fd, const void *buf, size_t count)
{
    size_t bytes_written = 0;
    while (bytes_written < count)
    {
        ssize_t ret = write(fd, (const char *)buf + bytes_written, count - bytes_written);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue; // Interrupted, try again
            }
            return -1; // ERROR
        }
        bytes_written += ret;
    }
    return 0;
}

string_t *read_string(int fd)
{
    uint32_t network_length;

    // Read length (4 bytes)
    if (read_all(fd, &network_length, sizeof(network_length)) == -1)
    {
        return NULL; // Error or EOF
    }

    // Convert network length to host
    uint32_t host_length = be32toh(network_length);

    // Allocate the main structure
    string_t *s = malloc(sizeof(string_t));
    if (s == NULL)
    {
        return NULL; // Allocation error
    }
    s->length = host_length;

    // Handle strings of length 0
    if (host_length == 0)
    {
        s->data = NULL; // No data to allocate or read
    }
    else
    {
        // Allocate buffer for datas
        s->data = malloc(host_length);
        if (s->data == NULL)
        {
            free(s); // Clean in case of failure
            return NULL;
        }

        // Read datas
        if (read_all(fd, s->data, host_length) == -1)
        {
            free(s->data); // Clean in case of failure
            free(s);
            return NULL;
        }
    }
    return s;
}

int write_string(int fd, const string_t *s)
{
    if (s == NULL)
    {
        return -1; // Safety
    }

    // Convert length of host to big-endian
    uint32_t network_length = htobe32(s->length);

    // Write length (4 bytes)
    if (write_all(fd, &network_length, sizeof(network_length)) == -1) {
        return -1; // Writing Error
    }

    // Write datas (only if length > 0)
    if (s->length > 0) {
        if (write_all(fd, s->data, s->length) == -1) {
            return -1; // Writing Error
        }
    }
    
    return 0; // Success
}

void free_my_string(string_t* s) {
    if (s != NULL) {
        free(s->data); // free(NULL) is safe, don't need test here
        free(s);
    }
}