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

    dprintf(STDOUT_FILENO, "the input is %s\n", input);

    if(code != CR && code != CB){

        uint64_t task_id = 0;

        if(code == LS || code == TM){

            if (input && strlen(input) != 0) {
                dprintf(STDERR_FILENO, "ERROR: -q and -l takes no argument\n");
                return -1;
            }
        }
        else{
            if (!input || strlen(input) == 0) {
                dprintf(STDERR_FILENO, "ERROR: this request requires an argument\n");
                return -1;
            }
            errno = 0;
            char *end;

            unsigned long long tmp = strtoull(input, &end, 10);

            if (errno == ERANGE) {
                dprintf(STDERR_FILENO, "ERROR : Capacity overflow.\n");
                return -1;
            }
            if (*end != '\0') {
                dprintf(STDERR_FILENO, "ERROR: invalid argument at client_handle_command\n");
                return -1;
            }
            task_id = (uint64_t) tmp;
        }

        // Creation of the request :
        simple_request_t* request = create_simple_request(code, task_id);

        if(request == NULL){
            return -1;
        }

        // Sending the request : 
        if(client_send_simple(request) < 0){
            dprintf(STDERR_FILENO, "Error : an error occured while sending an simple request\n");
            return -1;
        }

        // Get the response
        void* ans = client_recv_answer(code);

        if (ans == NULL) {
            dprintf(STDERR_FILENO, "Error receiving answer\n");
            return -1;
        }
        // Print the answer
        tadmor_print_response(code, ans);
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
static char *reconstruct_arg(int argc, char **argv, int start)
{
    if (start >= argc)
        return NULL;

    // Calculer la taille nécessaire (sans espaces)
    size_t total = 0;
    for (int i = start; i < argc; i++) {
        for (size_t j = 0; j < strlen(argv[i]); j++) {
            if (argv[i][j] != ' ')
                total++;
        }
    }

    if (total == 0)
        return NULL;

    char *out = malloc(total + 1);
    if (!out) {
        perror("malloc");
        return NULL;
    }

    size_t pos = 0;
    for (int i = start; i < argc; i++) {
        for (size_t j = 0; j < strlen(argv[i]); j++) {
            if (argv[i][j] != ' ')
                out[pos++] = argv[i][j];
        }
    }

    out[pos] = '\0';
    return out;
}

/**
 * @brief handle the argument
 */
static int argument_handler(uint16_t opcode, int pipe_rename, int argc, char** argv){

    char* input = reconstruct_arg(argc, argv, optind);
    int res = 0;

    if (input == NULL){
        input = strdup("");
    }
        
    if (!input) {
        perror("strdup");
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
    int opcode = 0;
    int pipe_rename = 0;
    int have_P =0;
    char pipedir[PATH_MAX] = {0};

    // Handle the differents arguments
    //TODO rajouté les options qu'il manque
    while ((opt = getopt(argc, argv, "qlxorep:P:")) != -1) {
        switch (opt) {
            case 'c': //TODO jalon-3
                break;
            case 's': //TODO jalon-3
                break;
            case 'n': //TODO jalon-3
                break;
            case 'r': opcode = RM; break;
            case 'q': opcode = TM; break;
            case 'l': opcode = LS; break;
            case 'x': opcode = TX; break;
            case 'o': opcode = SO; break;
            case 'e': opcode = SE; break;
            case 'p': pipe_rename = 1; break;
            case 'P':
                strncpy(pipedir, optarg, sizeof(pipedir)-1);
                pipedir[sizeof(pipedir)-1] = '\0';
                have_P = 1;
                break;
            default:
                dprintf(STDERR_FILENO, "Invalid option \n");
                return EXIT_FAILURE;
        }
    }
    if (have_P) {
        // override pipe_path directement
        strncpy(pipe_path, pipedir, sizeof(pipe_path)-1);
        pipe_path[sizeof(pipe_path)-1] = '\0';
    } else {
        if (pipe_file_read() < 0) {
            dprintf(STDERR_FILENO, "LAUNCH ERRAID BEFORE TADMOR !!!\n");
            return EXIT_FAILURE;
        }
    }
    if (!pipe_rename && opcode == 0) {
        dprintf(STDERR_FILENO, "No command specified\n");
        return EXIT_FAILURE;
    }
    return argument_handler(opcode, pipe_rename, argc, argv);
}