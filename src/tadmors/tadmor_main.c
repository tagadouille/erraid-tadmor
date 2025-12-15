#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "tadmor.h"
#include "communication/answer.h"
#include "communication/code.h"
#include "communication/request.h"
#include "communication/communication.h"
#include "communication/pipes.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* --------------------------------------------------------------
 * Mini-parser: take the full command line and dispatch it
 * -------------------------------------------------------------- */
static int client_handle_command(uint16_t code, const char *input){

    if(code != CR && code != CB){

        uint64_t task_id = 0;

        if(strlen(input) != 0){
            //Convert the input to uint64_t :
            errno = 0;
            char *end;

            unsigned long long tmp = strtoull(input, &end, 10);

            if (errno == ERANGE) {
                dprintf(STDERR_FILENO, "ERROR : Capacity overflow.\n");
                return -1;
            }
            if (*end != '\0') {
                dprintf(STDERR_FILENO, "ERROR: invalid argument\n");
                return -1;
            }
            task_id = (uint64_t) tmp;
        }
        else{
            if(code == TM){
                dprintf(STDERR_FILENO, "ERROR: -q takes no argument\n");
                return -1;
            }
        }

        // Création of the request
        simple_request_t* request = create_simple_request(code, task_id);

        if(request == NULL){
            return -1;
        }

        //!Provisoire :
        dprintf(STDOUT_FILENO, "Une requête de type %u et de id %zu a été faite et va être envoyé\n", request -> opcode, request -> task_id);

        // Sending the request
        answer_t ans;
        int has_task = 1;

        if(code == RM || code == TM){
            has_task = 0;
        }

        if(client_send_simple(request, &ans, has_task) < 0){
            dprintf(STDERR_FILENO, "Error : an error occured while sending an simple request\n");
            return -1;
        }
        tadmor_print_answer(&ans);
        free(request);
    }
    else{
        //TODO requête complexe jalon-3
    }
    return 0;
}

/* --------------------------------------------------------------
 * Reconstruct the remaining arguments into one string
 * -------------------------------------------------------------- */
static char* reconstruct_arg(int argc, char** argv){
    
    char* input = malloc(512);

    if(input == NULL){
        return NULL;
    }
    size_t pos = 0;

    for (int i = optind; i < argc; i++) {

        size_t len = strlen(argv[i]);

        if (pos + len + 2 >= sizeof(input)){
            break;
        }

        memcpy(input + pos, argv[i], len);
        pos += len;
        input[pos++] = ' ';
    }
    return input;
}

static int argument_handler(int opt, int argc, char** argv){
    uint16_t opcode = 0;
    int pipe_rename = 0;

    switch (opt){
        case 'c':
            //TODO jalon-3
            break;
        case 's':
            //TODO jalon-3
            break;
        case 'n':
            //TODO jalon-3
            break;
        case 'r':
            //TODO jalon-3
            opcode = RM;
            break;
        case 'l':
            opcode = LS;
            break;
        case 'x':
            opcode = TX;
            break;
        case 'o':
            opcode = SO;
            break;
        case 'e':
            opcode = SE;
            break;
        case 'p':
            pipe_rename = 1;
            break;
        case 'q':
            //TODO jalon-3
            opcode = TM;
            break;
        
        default:
            dprintf(STDERR_FILENO, "Invalid argument \n");
            return -1;
    }

    char* input = reconstruct_arg(argc, argv);
    int res = 0;

    if(input == NULL){
        return -1;
    }
    if(pipe_rename == 1){
        res = pipe_path_rename(input);
    }
    else{
        res = client_handle_command(opcode, input);
    }
    
    free(input);
    return res;
}

/* --------------------------------------------------------------
 * main
 * -------------------------------------------------------------- */
int main(int argc, char **argv)
{
    int opt;

    // Handle the differents arguments
    while ((opt = getopt(argc, argv, "qe:o:x:lr:c:s:p:e:")) != -1) {
        argument_handler(opt, argc, argv);
    }

    return 0;
}