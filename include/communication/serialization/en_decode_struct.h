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

/* --------------------- helper: build commandline string ----------
 * Protocol LIST wants TASK.N.COMMANDLINE as a single string.
 * If command is simple (SI) we join argv with spaces.
 * If command is composed (SQ) we flatten subcommands separated by " | " (or " ; ").
 * We return a freshly malloced string_t (filled) or NULL on error.
 * Caller must free->data and struct.
 -----------------------------------------------------------------*/
string_t *command_to_commandline(const command_t *cmd);