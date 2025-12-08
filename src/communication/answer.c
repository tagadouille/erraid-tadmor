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
    
    if(anstype == OK){
        answer -> task_id = task_id;
    }
    else if(anstype == ERR){
        if(errcode != NF && errcode != NR){
            dprintf(STDERR_FILENO, "Invalid errcode\n");
            free(answer);
            return NULL;
        }
        answer -> errcode = errcode;
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
    a_list -> nbtask = nbtask;
    a_list -> all_task = all_task;

    return a_list;
    
}

a_timecode_t* create_a_timecode_t(uint16_t anstype, uint32_t nbrun, time_exitcode_t* all_timecode){

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
    time -> nbrun = nbrun;
    time -> all_timecode = all_timecode;

    return time;
}

a_output_t* create_a_output_t(uint16_t anstype, string_t output, uint16_t errcode) {

    if (anstype != OK && anstype != ERR) {
        dprintf(STDERR_FILENO, "Invalid anstype\n");
        return NULL;
    }

    if (anstype == ERR && (errcode != NF && errcode != NR)) {
        dprintf(STDERR_FILENO, "Invalid errcode\n");
        return NULL;
    }

    a_output_t* a_output = malloc(sizeof(a_output_t));
    if (!a_output) {
        perror("malloc");
        return NULL;
    }

    a_output->anstype = anstype;
    a_output->errcode = errcode;
    a_output->output = output; // si output est déjà alloué ailleurs

    return a_output;
}



void free_answer(answer_t* answer){
    if(answer != NULL){
        free(answer);
    }
}

void free_a_list(a_list_t* list) {
    if (list == NULL){
        return;
    }

    free(list->all_task);
    free(list);
}



void free_a_timecode_t(a_timecode_t* timecode){
    if(timecode == NULL) return;

    for (size_t i = 0; i < timecode -> nbrun; i++){

        if(timecode -> all_timecode != NULL){
            free(timecode -> all_timecode);
        }
    }
    free(timecode);
}

void free_a_output_t(a_output_t* output){
    if(output == NULL) return;

    string_free(&(output -> output));
    free(output);
}