#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>
#include <stddef.h>

#include "communication/request.h"
#include "communication/answer.h"
#include "types/timing.h"
#include "types/task.h"
#include "types/argument.h"
#include "types/my_string.h"

/* ============================================================
 * Safety limits to avoid malicious or corrupted serialized data
 * ============================================================ */
#define LIMIT_MAX_STR_LEN   (1u << 20)   /* Maximum string size: 1 MiB */
#define LIMIT_MAX_ARGC      10000u       /* Maximum number of arguments */
#define LIMIT_MAX_CMDS      10000u       /* Maximum number of commands */
#define LIMIT_MAX_TASKS     100000u      /* Maximum number of tasks */
#define LIMIT_MAX_RUNS      100000u      /* Maximum number of executions */

/* ============================================================
 * LOW-LEVEL INTEGER SERIALIZATION (BIG-ENDIAN)
 * ============================================================ */

/**
 * @brief Encode an unsigned 8-bit integer.
 */
int encode_uint8(int fd, uint8_t v);

/**
 * @brief Decode an unsigned 8-bit integer.
 */
int decode_uint8(int fd, uint8_t *v);

/**
 * @brief Encode an unsigned 16-bit integer.
 */
int encode_uint16(int fd, uint16_t v);

/**
 * @brief Decode an unsigned 16-bit integer.
 */
int decode_uint16(int fd, uint16_t *v);

/**
 * @brief Encode an unsigned 32-bit integer.
 */
int encode_uint32(int fd, uint32_t v);

/**
 * @brief Decode an unsigned 32-bit integer.
 */
int decode_uint32(int fd, uint32_t *v);

/**
 * @brief Encode an unsigned 64-bit integer.
 */
int encode_uint64(int fd, uint64_t v);

/**
 * @brief Decode an unsigned 64-bit integer.
 */
int decode_uint64(int fd, uint64_t *v);

/**
 * @brief Encode a signed 32-bit integer.
 */
int encode_int32(int fd, int32_t v);

/**
 * @brief Decode a signed 32-bit integer.
 */
int decode_int32(int fd, int32_t *v);

/**
 * @brief Encode a signed 64-bit integer.
 */
int encode_int64(int fd, int64_t v);

/**
 * @brief Decode a signed 64-bit integer.
 */
int decode_int64(int fd, int64_t *v);

/* ============================================================
 * LOW-LEVEL I/O HELPERS
 * ============================================================ */

/**
 * @brief Read exactly size bytes from a file descriptor.
 *
 * @return 0 on success, -1 on error or EOF.
 */
int read_full(int fd, void *buf, size_t size);

/**
 * @brief Write exactly size bytes to a file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int write_full(int fd, const void *buf, size_t size);

#endif
