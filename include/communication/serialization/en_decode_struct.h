#include "communication/answer.h"

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

int encode_command(int fd, const command_t *cmd);
int decode_command(int fd, command_t **out);