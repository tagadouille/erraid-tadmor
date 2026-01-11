#ifndef ANSWER_H
#define ANSWER_H

#include "communication/code.h"
#include <stdint.h>
#include "types/time_exitcode.h"
#include "types/task.h"

/**
 * For the basic answer and for the
 * errors
 */
typedef struct{

    uint16_t anstype;
    uint64_t task_id;
    uint16_t errcode;

}answer_t;

/**
 * For the answer to STDOUT and STDERR
 */
typedef struct{

    uint16_t anstype;
    string_t* output;
    uint16_t errcode;

}a_output_t;

/**
 * For the answer to time_exitcode
 */
typedef struct{

    uint16_t anstype;
    uint16_t errcode;
    time_array_t time_arr;

}a_timecode_t;

/**
 * For the answer to list
 */
typedef struct{

    uint16_t anstype;
    all_task_t all_task;

}a_list_t;

/**
 * @brief fill and create the answer structure
 * @param anstype the type of the answer
 * @param task_id the id of the task
 * @param errcode the error code if there's an error
 */
answer_t* create_answer(uint16_t anstype, uint64_t task_id, uint16_t errcode);

/**
 * @brief fill and create the answer list structure
 * @param anstype the type of the answer
 * @param nbtask the number of task
 * @param all_task an array of task of size nbtask
 */
a_list_t* create_a_list(uint16_t anstype, all_task_t* all_task_data);

/**
 * @brief fill and create the answer timecode structure
 * @param anstype the type of the answer
 * @param nbrun the number of time exitcode
 * @param all_timecode an array of time_exitcode_t of size nbrun
 */
a_timecode_t* create_a_timecode(uint16_t anstype, uint32_t nbrun, time_exitcode_t* all_timecode);

/**
 * @brief fill and create the answer output structure
 * @param anstype the type of the answer
 * @param output the output of a task
 * @param errcode the error code if there's an error
 */
a_output_t* create_a_output(uint16_t anstype, string_t* output, uint16_t errcode);

/**
 * @brief free the answer structure
 */
void free_answer(answer_t* answer);

/**
 * @brief free the answer list structure
 */
void free_a_list(a_list_t* list);

/**
 * @brief free the answer timecode structure
 */
void free_a_timecode(a_timecode_t* timecode);

/**
 * @brief free the answer output structure
 */
void free_a_output(a_output_t* output);

#endif