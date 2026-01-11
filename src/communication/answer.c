#define _POSIX_C_SOURCE 200809L

#include "communication/answer.h"
#include "communication/code.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

answer_t* create_answer(uint16_t anstype, uint64_t task_id, uint16_t errcode){
    
    if(anstype != OK && anstype != ERR){
        dprintf(STDERR_FILENO, "Invalide anstype \n");
        return NULL;
    }
    
    answer_t* answer = malloc(sizeof(answer_t));
        
    if(answer == 0){
        perror("malloc");
        return NULL;
    }
    
    answer->anstype = anstype;
    
    if(anstype == OK){
        answer->task_id = task_id;
        answer->errcode = 0;
    }
    else if(anstype == ERR){
        if(errcode != NF && errcode != NR){
            dprintf(STDERR_FILENO, "Invalid errcode\n");
            free(answer);
            return NULL;
        }
        answer->errcode = errcode;
        answer->task_id = 0;
    }
    
    return answer;
}

a_list_t* create_a_list(uint16_t anstype, uint32_t nbtask, task_t* all_task){

    if(anstype != OK){
        dprintf(STDERR_FILENO, "The anstype must be OK\n");
        return NULL;
    }
    a_list_t* a_list = malloc(sizeof(a_list_t));

    if(a_list == NULL){
        perror("malloc");
        return NULL;
    }

    a_list->all_task = malloc(sizeof(all_task_t));
    if (a_list->all_task == NULL) {
        perror("malloc");
        free(a_list);
        return NULL;
    }

    a_list->all_task->nbtask = nbtask;
    a_list->all_task->all_task = all_task;

    return a_list;
}

a_timecode_t* create_a_timecode(uint16_t anstype, uint32_t nbruns, time_exitcode_t* all_timecode){

    if(anstype != ERR && anstype != OK){
        dprintf(STDERR_FILENO, "The anstype is incorrect\n");
        return NULL;
    }
    a_timecode_t* time = malloc(sizeof(a_timecode_t));

    if(time == NULL){
        return NULL;
    }

    time -> anstype = anstype;

    if(anstype == ERR){
        time -> errcode = NF;
        return time;
    }
    time -> time_arr.nbruns = nbruns;
    time -> time_arr.all_timecode = all_timecode;

    return time;
}

a_output_t* create_a_output(uint16_t anstype, string_t* output, uint16_t errcode)
{
    a_output_t* a_output = calloc(1, sizeof(a_output_t));
    if (!a_output) {
        perror("calloc create_a_output_t");
        return NULL;
    }

    a_output->anstype = anstype;
    a_output->errcode = errcode;

    a_output->output = string_copy(output);
    if (!a_output->output) {
        dprintf(STDERR_FILENO, "create_a_output_t: string_copy failed\n");
        free(a_output);
        return NULL;
    }

    return a_output;
}

void free_answer(answer_t* answer){
    if(answer != NULL){
        free(answer);
    }
}

void free_a_list(a_list_t* list) {

    if (!list || !list->all_task->all_task)
        return;

    free_all_task(list->all_task);

    free(list);
}

void free_a_timecode(a_timecode_t* timecode){

    if (timecode == NULL) {
        return;
    }

    if (timecode->time_arr.all_timecode != NULL) {
        free(timecode->time_arr.all_timecode);
    }
    
    free(timecode);
}

void free_a_output(a_output_t* output){
    if(output == NULL) return;

    string_free(output->output);
    free(output);
}