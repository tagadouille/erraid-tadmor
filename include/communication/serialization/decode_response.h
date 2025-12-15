#include "communication/answer.h"

/* LIST: OK + NBTASK + tasks */

a_list_t* decode_a_list(int fd);

/* STDOUT / STDERR: OK + string OR ER + errcode */

a_output_t* decode_a_output(int fd);

/* TIMES_EXITCODES: OK + runs[] OR ER + errcode */

a_timecode_t* decode_a_timecode(int fd);

answer_t* decode_answer(int fd);