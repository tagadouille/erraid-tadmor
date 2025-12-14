#include "communication/answer.h"

/* ERROR: ANSTYPE='ER' + ERRCODE(uint16) */
int encode_answer_err(int fd, uint16_t errcode);


/* OK with TASKID (CREATE, COMBINE) */
int encode_answer_ok_taskid(int fd, uint64_t taskid);

int encode_answer_ok_nopayload(int fd);

int encode_a_list(int fd, const a_list_t *ans);

int encode_a_output(int fd, const a_output_t *ans);

int encode_a_timecode(int fd, const a_timecode_t *ans);