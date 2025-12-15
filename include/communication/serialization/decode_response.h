#include "communication/answer.h"

int decode_answer_err(int fd, uint16_t *errcode_out);

int decode_answer_ok_taskid(int fd, uint64_t *taskid_out);

/* OK with no payload (REMOVE, TERMINATE) */

int decode_answer_ok_nopayload(int fd);

/* LIST: OK + NBTASK + tasks */

int decode_a_list(int fd, a_list_t *ans);

/* STDOUT / STDERR: OK + string OR ER + errcode */

int decode_a_output(int fd, a_output_t *ans);

/* TIMES_EXITCODES: OK + runs[] OR ER + errcode */

int decode_a_timecode(int fd, a_timecode_t *ans);

int decode_answer(int fd, const answer_t *ans);