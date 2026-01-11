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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "tadmor.h"
#include "communication/answer.h"
#include "communication/code.h"
#include "communication/request.h"
#include "communication/communication.h"
#include "communication/pipes.h"
#include "types/task.h"
#include "erraids/erraid-helper.h"

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
    char *hours_str, char *days_of_week_str, int is_abstract,
    command_type_t combination_type, int argc, char **argv, int optind){

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
        // Handle complex request for task creation or combination

        // Initialize timing and command structures
        timing_t* timing = timing_create_from_strings(minutes_str, hours_str, days_of_week_str);

        if(timing == NULL){
            dprintf(STDERR_FILENO, "[client_handle_command] Error creating timing structure\n");
            return -1;
        }

        if (is_abstract) {
            timing_set_abstract(timing);
        }

        command_t* command = NULL;
        composed_t* composed = NULL;

        if (code == CR) {
            // Reconstruct the command string from remaining arguments
            command = command_create_from_string(input);
            if(command == NULL){
                dprintf(STDERR_FILENO, "[client_handle_command] Error creating command from string\n");
                timing_free(timing);
                return -1;
            }
        } else { // CB
            int nb_tasks = argc - optind;

            if (nb_tasks <= 0) {
                dprintf(STDERR_FILENO, "ERROR: Combination request requires at least one task ID\n");
                timing_free(timing);
                return -1;
            }

            composed = malloc(sizeof(composed_t));
            if (!composed) {
                perror("malloc composed");
                timing_free(timing);
                return -1;
            }

            composed->task_ids = malloc(nb_tasks * sizeof(uint64_t));
            if (!composed->task_ids) {
                perror("malloc composed->task_id");
                free(composed);
                timing_free(timing);
                return -1;
            }
            composed->nb_task = nb_tasks;
            composed->type = combination_type;

            // Parse task IDs
            for (int i = 0; i < nb_tasks; i++) {
                char *end;
                errno = 0;
                unsigned long long tmp = strtoull(argv[optind + i], &end, 10);

                if (errno == ERANGE || *end != '\0') {
                    dprintf(STDERR_FILENO, "ERROR: Invalid task ID: %s\n", argv[optind + i]);
                    free(composed->task_ids);
                    free(composed);
                    timing_free(timing);
                    return -1;
                }
                composed->task_ids[i] = (uint64_t)tmp;
            }

            // Verifications specific to combination type :
            if(composed->type == IF && (composed->nb_task < 2 || composed->nb_task > 3)) {
                dprintf(STDERR_FILENO, "Error: IF combination requires exactly 2-3 tasks.\n");
                return -1;
            }

            if(composed->type != IF && composed->nb_task < 2) {
                dprintf(STDERR_FILENO, "Error: PIPE combination requires at least 2 tasks.\n");
                return -1;
            }
        }

        //Complex request creation and sending
        complex_request_t* request = create_complex_request(code, timing, command, composed);

        if(request == NULL){
            dprintf(STDERR_FILENO, "[client_handle_command] Error creating complex request\n");
            if (command) command_free(command);
            // composed is freed inside create_complex_request in case of error
            timing_free(timing);
            return -1;
        }

        // Sending the request : 
        if(client_send_complex(request) < 0){
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
    return 0;
}

/**
 * @brief Reconstruct the remaining arguments into a single string with spaces.
 * @param argc number of arguments
 * @param argv array of arguments
 * @param start index to start from
 * @return reconstructed string with spaces, or NULL if no arguments
 */
static char *reconstruct_command_string(int argc, char **argv, int start)
{
    if (start >= argc) {
        dprintf(2, "[reconstruct_command_string] No arguments to reconstruct\n");
        return NULL;
    }

    // Calculate the total length needed, including spaces
    size_t total_len = 0;
    for (int i = start; i < argc; i++) {
        total_len += strlen(argv[i]);
    }

    // Add space for spaces between arguments and the null terminator
    if (argc - start > 1) {
        total_len += (argc - start - 1);
    }

    char *out = malloc(total_len + 1);
    if (!out) {
        perror("malloc");
        return NULL;
    }

    // Concatenate arguments with spaces
    char *current_pos = out;
    for (int i = start; i < argc; i++) {

        strcpy(current_pos, argv[i]);
        current_pos += strlen(argv[i]);

        if (i < argc - 1) {
            *current_pos = ' ';
            current_pos++;
        }
    }
    *current_pos = '\0';

    return out;
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

    // Fill the output string without spaces

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
    char *minutes_str, char *hours_str, char *days_of_week_str,
    int is_abstract, command_type_t combination_type){

    char* input = NULL;

    if(opcode == CR){
        input = reconstruct_command_string(argc, argv, optind);
    } 
    else if (opcode != CB) {
        input = reconstruct_arg(argc, argv, optind);
    }
    
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

        switch (fork())
        {
        case -1:
            perror("fork");
            return -1;
        
        case 0:
            execlp("killall", "killall", "-SIGCHLD", "erraid", NULL);
            perror("execlp");
            return -1;
            break;
        }
        wait(NULL);
    }
    else{
        res = client_handle_command(opcode, input, minutes_str, hours_str,
             days_of_week_str, is_abstract, combination_type, argc, argv, optind);
    }
    
    free(input);
    return res;
}

/* --------------------------------------------------------------
 * main
 * -------------------------------------------------------------- */
int main(int argc, char **argv) {

    int opt;
    uint16_t opcode = 0;

    int pipe_rename = 0;

    // Task creation var :
    char *minutes_str = NULL;
    char *hours_str = NULL;
    char *days_of_week_str = NULL;

    int is_abstract = 0;
    command_type_t combination_type = 0;
    int combination_flag = 0;

    if(pipe_file_read() < 0){
        dprintf(STDERR_FILENO, "You must launch erraid before tadmor. \n");
        return -1;
    }

    // Handle the differents arguments
    while ((opt = getopt(argc, argv, "cm:H:d:nqlxorePsip")) != -1) {
        switch (opt) {
            case 'c': 
                opcode = CR;
                // The command is the remaining part of the arguments
                break;
            case 's':
                if (combination_flag) {
                    dprintf(STDERR_FILENO, "ERROR: Options -s, -i, and -p cannot be combined.\n");
                    return EXIT_FAILURE;
                }
                opcode = CB;
                combination_type = SQ;
                combination_flag = 1;
                break;
            case 'i':
                if (combination_flag) {
                    dprintf(STDERR_FILENO, "ERROR: Options -s, -i, and -p cannot be combined.\n");
                    return EXIT_FAILURE;
                }
                opcode = CB;
                combination_type = IF;
                combination_flag = 1;
                break;
            case 'p':
                if (combination_flag) {
                    dprintf(STDERR_FILENO, "ERROR: Options -s, -i, and -p cannot be combined.\n");
                    return EXIT_FAILURE;
                }
                opcode = CB;
                combination_type = PL;
                combination_flag = 1;
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
            case 'q': opcode = TM; break;
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

    if (opcode == CR && combination_flag) {
        dprintf(STDERR_FILENO, "ERROR: Option -c cannot be combined with -s, -i, or -p.\n");
        return EXIT_FAILURE;
    }

    if (is_abstract && (minutes_str || hours_str || days_of_week_str)) {
        dprintf(STDERR_FILENO, "ERROR: -n option cannot be used with -m, -H, or -d\n");
        return EXIT_FAILURE;
    }

    if (!pipe_rename && opcode == 0) {
        dprintf(STDERR_FILENO, "No command specified\n");
        return EXIT_FAILURE;
    }

    return argument_handler(opcode, pipe_rename, argc, argv, minutes_str, hours_str, days_of_week_str, is_abstract, combination_type);
}