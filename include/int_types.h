#ifndef INT_TYPES_H
#define INT_TYPES_H

#include <stdint.h>
#include <stdlib.h>


/**
 * @brief Read a uint16 (Big-Endian) from a file descriptor.
 * @param fd The file descriptor
 * @param value Pointer where the result should be stored
 * @return 0 on success, -1 on error
 */
int read_uint16(int fd, uint16_t *value);
int write_uint16(int fd, uint16_t value);

/**
 * @brief Read a uint32 (Big-Endian) from a file descriptor.
 * @param fd file descriptor
 * @param value Pointer where the result should be stored
 * @return 0 on success, -1 on error
 */
int read_uint32(int fd, uint32_t *value);
int write_uint32(int fd, uint32_t value);

/**
 * @brief Read a uint64 (Big-Endian) from a file descriptor.
 * @param fd file descriptor
 * @param value Pointer where the result should be stored
 * @return 0 on success, -1 on error
 */
int read_uint64(int fd, uint64_t *value);
int write_uint64(int fd, uint64_t value);


/**
 * @brief Read an int16 (Big-Endian, 2's complement) from a file descriptor.
 * @param fd file descriptor
 * @param value Pointer where the result should be stored
 * @return 0 on success, -1 on error
 */
int read_int16(int fd, int16_t *value);
int write_int16(int fd, int16_t value);

/**
 * @brief Read an int32 (Big-Endian, 2's complement) from a file descriptor.
 * @param fd file descriptor
 * @param value Pointer where the result should be stored
 * @return 0 on success, -1 on error
 */
int read_int32(int fd, int32_t *value);
int write_int32(int fd, int32_t value);

/**
 * @brief Read an int64 (Big-Endian, 2's complement) from a file descriptor.
 * @param fd file descriptor
 * @param value Pointer where the result should be stored
 * @return 0 on success, -1 on error
 */
int read_int64(int fd, int64_t *value);
int write_int64(int fd, int64_t value);

#endif