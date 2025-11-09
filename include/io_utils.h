#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stddef.h>
#include <stdint.h>
//#include <endian.h>

/**
 * @brief Read exactly 'count' bytes from 'fd' into 'buf'.
 * Handls partial reads and interruptions.
 * @param fd file descriptor
 * @param buf buffer to read into
 * @param count number of bytes to read
 * @return 0 on success, -1 on error or premature EOF
 */
int read_all(int fd, void* buf, size_t count);

/**
 * @brief Write exactly 'count' bytes from 'buf' to 'fd'.
 * Handle partial writes and interruptions.
 * @param fd file descriptor
 * @param buf buffer to write from
 * @param count number of bytes to write
 * @return 0 on success, -1 on error
 */
int write_all(int fd, const void* buf, size_t count);

#endif