#include "communication/answer.h"

int encode_a_list(int fd, const a_list_t *ans);

int encode_a_output(int fd, const a_output_t *ans);

int encode_a_timecode(int fd, const a_timecode_t *ans);

int encode_answer(int fd, const answer_t *ans);