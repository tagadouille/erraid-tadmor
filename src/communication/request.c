#include "communication/code.h"
#include "communication/request.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

simple_request_t* create_simple_request(uint16_t opcode, uint64_t task_id){
    
    simple_request_t* request = malloc(sizeof(simple_request_t));

    if(request == NULL){
        perror("malloc");
        return NULL;
    }
    request -> opcode = opcode;
    request -> task_id = task_id;

    return request;
}

void free_simple_request(simple_request_t* request){
    if(request == NULL){
        return;
    }
    free(request);
}

//TODO jalon-3
/*
complex_request_t* create_complex_request(uint16_t opcode, timing_t* timing, command_t* command, composed_t* composed){
    return NULL;
}


void free_complex_request(complex_request_t* request){
    
    if(request == NULL){
        return;
    }
}*/