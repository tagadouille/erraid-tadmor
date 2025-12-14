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

/* Safety limits to avoid malicious / corrupted sizes */
#define LIMIT_MAX_STR_LEN   (1u << 20)   /* 1 MiB */
#define LIMIT_MAX_ARGC      10000u
#define LIMIT_MAX_CMDS      10000u
#define LIMIT_MAX_TASKS     100000u
#define LIMIT_MAX_RUNS      100000u

/* ============================================================
 * LOW-LEVEL INTEGER ENCODING (BIG-ENDIAN)
 * ============================================================ */
int encode_uint8(int fd, uint8_t v);
int decode_uint8(int fd, uint8_t *v);

int encode_uint16(int fd, uint16_t v);
int decode_uint16(int fd, uint16_t *v);

int encode_uint32(int fd, uint32_t v);
int decode_uint32(int fd, uint32_t *v);

int encode_uint64(int fd, uint64_t v);
int decode_uint64(int fd, uint64_t *v);

int encode_int32(int fd, int32_t v);
int decode_int32(int fd, int32_t *v);

int encode_int64(int fd, int64_t v);
int decode_int64(int fd, int64_t *v);

int read_full(int fd, void *buf, size_t size);

int write_full(int fd, const void *buf, size_t size);

#endif // SERIALIZATION_H
