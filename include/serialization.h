#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>
#include <stddef.h>

#include "request.h"
#include "answer.h"
#include "types/timing.h"
#include "types/task.h"
#include "types/arguments.h"
#include "types/my_string.h"

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

/* ============================================================
 * STRING ENCODING (string_t)
 * format: LENGTH(uint32) + DATA[length]
 * ============================================================ */
int encode_string(int fd, const string_t *s);
int decode_string(int fd, string_t *s);

/* ============================================================
 * TIMING ENCODING
 * MINUTES(uint64), HOURS(uint32), DAYS(uint8)
 * ============================================================ */
int encode_timing(int fd, const timing_t *t);
int decode_timing(int fd, timing_t *t);

/* ============================================================
 * ARGUMENTS (ARGV)
 * ARGC(uint32) + ARGV[i] = string
 * ============================================================ */
int encode_arguments(int fd, const arguments_t *args);
int decode_arguments(int fd, arguments_t *args);

/* ============================================================
 * SIMPLE REQUEST (LS, RM, SO, SE, TX, TM)
 * OPCODE(uint16) + TASKID(optional uint64)
 * ============================================================ */
int encode_simple_request(int fd, const simple_request_t *req);
int decode_simple_request(int fd, simple_request_t *req);

/* ============================================================
 * COMPLEX REQUEST (CR, CB)
 * CR: OPCODE + TIMING + ARGUMENTS
 * CB: OPCODE + TIMING + TYPE(uint16)
 *          + NBTASKS(uint32) + TASKID[]
 * ============================================================ */
int encode_complex_request(int fd, const complex_request_t *req);
int decode_complex_request(int fd, complex_request_t *req);

/* ============================================================
 * ANSWERS (daemon -> client)
 * ============================================================ */

/* ERROR: ANSTYPE='ER' + ERRCODE(uint16) */
int encode_answer_err(int fd, uint16_t errcode);
int decode_answer_err(int fd, uint16_t *errcode_out);

/* OK with TASKID (CREATE, COMBINE) */
int encode_answer_ok_taskid(int fd, uint64_t taskid);
int decode_answer_ok_taskid(int fd, uint64_t *taskid_out);

/* OK with no payload (REMOVE, TERMINATE) */
int encode_answer_ok_nopayload(int fd);
int decode_answer_ok_nopayload(int fd);

/* LIST: OK + NBTASK + tasks */
int encode_a_list(int fd, const a_list_t *ans);
int decode_a_list(int fd, a_list_t *ans);

/* STDOUT / STDERR: OK + string OR ER + errcode */
int encode_a_output(int fd, const a_output_t *ans);
int decode_a_output(int fd, a_output_t *ans);

/* TIMES_EXITCODES: OK + runs[] OR ER + errcode */
int encode_a_timecode(int fd, const a_timecode_t *ans);
int decode_a_timecode(int fd, a_timecode_t *ans);

#endif // SERIALIZATION_H
