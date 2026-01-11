#include "communication/answer.h"
/*
* @brief Encode an a_list_t structure.
* @param fd File descriptor to write to.
* @param ans Pointer to the a_list_t structure to encode.
* @return 0 on success, -1 on failure.
*/
int encode_a_list(int fd, const a_list_t *ans);

/*
* @brief Encode an a_output_t structure.
* @param fd File descriptor to write to.
* @param ans Pointer to the a_output_t structure to encode.
* @return 0 on success, -1 on failure.
*/
int encode_a_output(int fd, const a_output_t *ans);

/*
* @brief Encode an a_timecode_t structure.
* @param fd File descriptor to write to.
* @param ans Pointer to the a_timecode_t structure to encode.
* @return 0 on success, -1 on failure.
*/
int encode_a_timecode(int fd, const a_timecode_t *ans);

/*
* @brief Encode an answer_t structure.
* @param fd File descriptor to write to.
* @param ans Pointer to the answer_t structure to encode.
* @return 0 on success, -1 on failure.
*/
int encode_answer(int fd, const answer_t *ans);