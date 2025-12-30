/* serialization.c
 *
 * Serialization / deserialization
 *
 * - All numeric fields are big-endian on the wire.
 * - All I/O uses write_full / read_full to avoid partial reads/writes.
 * - Conforms to formats described in protocole.md:
 *     Requests: LS, CR, CB, RM, TX, SO, SE, TM
 *     Responses: OK / ER variants (LIST, CREATE, COMBINE, REMOVE, TIMES_EXITCODES, STDOUT/STDERR, TERMINATE)
 *
 * Note: this file assumes the following helper types exist (adjust if different):
 *   string_t { uint32_t length; char *data; }
 *   arguments_t { string_t *command; uint32_t argc; string_t **argv; }
 *   timing_t { uint64_t minutes; uint32_t hours; uint8_t daysofweek; }
 *   command_t { command_type_t type; union { arguments_t simple; struct { uint16_t count; command_t **cmds; } composed; } args; }
 *   a_list_t, a_timecode_t, a_output_t, answer_t, simple_request_t, complex_request_t exist as in your headers.
 *
 * If your real types differ, update small sections accordingly.
 */

#define _POSIX_C_SOURCE 200809L
#include "communication/serialization/serialization.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h> /* htons/htonl */
#include <stdint.h>
#include <limits.h>
#include <stdio.h>

/* --------------------- low-level robust I/O --------------------- */
/* returns 0 on success, -1 on error */
int write_full(int fd, const void *buf, size_t size)
{
    const unsigned char *p = buf;
    size_t off = 0;
    while (off < size) {
        ssize_t w = write(fd, p + off, size - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (w == 0) return -1; /* shouldn't happen for pipes when writing */
        off += (size_t)w;
    }
    return 0;
}

int read_full(int fd, void *buf, size_t size)
{
    unsigned char *p = buf;
    size_t off = 0;
    while (off < size) {
        dprintf(2, "[read_full] reading %zu bytes (off=%zu size=%zu)\n", size - off, off, size);
        ssize_t r = read(fd, p + off, size - off);
        if (r < 0) {
            dprintf(2, "[read_full] read error: %s\n", strerror(errno));
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0){ 
            dprintf(2, "[read_full] EOF reached\n");
            return -1; 
        }/* EOF => incomplete message */
        off += (size_t)r;
    }
    return 0;
}

/* --------------------- portable 64-bit BE helpers --------------------- */
static uint64_t hton64(uint64_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t lo = (uint32_t)(x & 0xFFFFFFFFu);
    hi = htonl(hi);
    lo = htonl(lo);
    return ((uint64_t)lo << 32) | hi;
#else
    return x;
#endif
}
static uint64_t ntoh64(uint64_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t lo = (uint32_t)(x & 0xFFFFFFFFu);
    hi = ntohl(hi);
    lo = ntohl(lo);
    return ((uint64_t)lo << 32) | hi;
#else
    return x;
#endif
}

/* --------------------- primitive encoders/decoders --------------------- */

int encode_uint8(int fd, uint8_t v) { return write_full(fd, &v, sizeof(v)); }
int decode_uint8(int fd, uint8_t *v) { return read_full(fd, v, sizeof(*v)); }

int encode_uint16(int fd, uint16_t v) {
    uint16_t be = htons(v);
    return write_full(fd, &be, sizeof(be));
}
int decode_uint16(int fd, uint16_t *v) {
    uint16_t be;
    if (read_full(fd, &be, sizeof(be)) < 0) return -1;
    *v = ntohs(be);
    return 0;
}

int encode_uint32(int fd, uint32_t v) {
    uint32_t be = htonl(v);
    return write_full(fd, &be, sizeof(be));
}
int decode_uint32(int fd, uint32_t *v) {
    uint32_t be;
    if (read_full(fd, &be, sizeof(be)) < 0) {
        dprintf(STDERR_FILENO, "Error : can't read uint32 from fd\n");
        return -1 ; 
    };
    *v = ntohl(be);
    return 0;
}

int encode_uint64(int fd, uint64_t v) {
    uint64_t be = hton64(v);
    return write_full(fd, &be, sizeof(be));
}
int decode_uint64(int fd, uint64_t *v) {
    uint64_t be;
    if (read_full(fd, &be, sizeof(be)) < 0) return -1;
    *v = ntoh64(be);
    return 0;
}

int encode_int64(int fd, int64_t v) {
    return encode_uint64(fd, (uint64_t)v);
}
int decode_int64(int fd, int64_t *v) {
    uint64_t u;
    if (decode_uint64(fd, &u) < 0) return -1;
    *v = (int64_t)u;
    return 0;
}

/* signed 32 bits support (if needed) */
int encode_int32(int fd, int32_t v) {
    uint32_t u = (uint32_t)v;
    return encode_uint32(fd, u);
}
int decode_int32(int fd, int32_t *v) {
    uint32_t u;
    if (decode_uint32(fd, &u) < 0) return -1;
    *v = (int32_t)u;
    return 0;
}