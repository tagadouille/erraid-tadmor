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

/** @brief Mini-parser: take the full command line and dispatch it
 * @param code the operation code
 * @param input the input string (argument)
 * @param minutes_str string representing minutes
 * @param hours_str string representing hours
 * @param days_of_week_str string representing days of the week
 * @param is_abstract whether the task is abstract
 * @return int 0 on success, -1 on failure
 */
static int client_handle_command(uint16_t code, const char *input, char *minutes_str,
     char *hours_str, char *days_of_week_str, int is_abstract){

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
        // Handle complex request for task creation
        dprintf(STDOUT_FILENO, "Handling complex request for task creation (CR)\n");

        // Initialize timing and command structures
        timing_t* timing = timing_create_from_strings(minutes_str, hours_str, days_of_week_str);
        
        if (is_abstract) {
            timing_set_abstract(timing);
        }
        
        dprintf(STDOUT_FILENO, "Timing structure initialized.\n");

        // The command string is in `input`
        // We need to create a command_t from it.
        // This part will be more complex, involving serialization.
        // For now, let's assume we have a function to do that.
        dprintf(STDOUT_FILENO, "Command to execute: %s\n", input);
        
        // Placeholder for command creation
        command_t command; // This should be created and filled.

        dprintf(STDOUT_FILENO, "timing and command variables initialized.\n");

        // The full implementation will involve creating a complex_request_t,
        // serializing the timing and command, and sending it to the daemon.
        timing_free(timing);
    }
    return 0;
}

/** @brief Reconstruct the remaining arguments into one string
 * @param argc number of arguments
 * @param argv array of arguments
 * @param start index to start from
 * @return reconstructed string without spaces, or NULL if no arguments
 */
static char *reconstruct_arg(int argc, char **argv, int start)
{
    if (start >= argc)
        return NULL;

    // Calculate the necessary size without space
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
 * @param opcode the operation code
 * @param pipe_rename whether to rename the pipe
 * @param argc number of arguments
 * @param argv array of arguments
 * @param minutes_str string representing minutes
 * @param hours_str string representing hours
 * @param days_of_week_str string representing days of the week
 * @param is_abstract whether the task is abstract
 * @return int 0 on success, -1 on failure
 */
static int argument_handler(uint16_t opcode, int pipe_rename, int argc, char** argv,
     char *minutes_str, char *hours_str, char *days_of_week_str, int is_abstract){

    char* input = NULL;

    if(opcode == CR){

    } 
    else input = reconstruct_arg(argc, argv, optind);
    
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
        res = client_handle_command(opcode, input, minutes_str, hours_str, days_of_week_str, is_abstract);
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
    char *minutes_str = NULL;
    char *hours_str = NULL;
    char *days_of_week_str = NULL;
    int is_abstract = 0;

    if(pipe_file_read() < 0){
        dprintf(STDERR_FILENO, "LAUNCH ERRAID BEFORE TADMOR !!! \n");
        return -1;
    }

    // Handle the differents arguments
    while ((opt = getopt(argc, argv, "c:m:H:d:nr:qlxoreP")) != -1) {
        switch (opt) {
            case 'c': 
                opcode = CR;
                // The command is the remaining part of the arguments
                break;
            case 'm': 
                minutes_str = optarg;
                break;
            case 'H': 
                hours_str = optarg;
                break;
            case 'd': 
                days_of_week_str = optarg;
                break;
            case 'n': 
                is_abstract = 1;
                break;
            case 'r': opcode = RM; break;
            case 'q': //TODO jalon-3
                opcode = TM; break;

            case 'l': opcode = LS; break;
            case 'x': opcode = TX; break;
            case 'o': opcode = SO; break;
            case 'e': opcode = SE; break;
            case 'P': pipe_rename = 1; break;
            
            default:
                dprintf(STDERR_FILENO, "Invalid option \n");
                return EXIT_FAILURE;
        }
    }

    if (is_abstract && (minutes_str || hours_str || days_of_week_str)) {
        dprintf(STDERR_FILENO, "ERROR: -n option cannot be used with -m, -H, or -d\n");
        return EXIT_FAILURE;
    }

    return argument_handler(opcode, pipe_rename, argc, argv, minutes_str, hours_str, days_of_week_str, is_abstract);
}