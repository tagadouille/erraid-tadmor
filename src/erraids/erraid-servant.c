#include "erraids/erraid-log.h"
#include "erraids/erraid.h"
#include "communication/answer.h"
#include "communication/request.h"
#include "communication/pipes.h"
#include "communication/communication.h"
#include "communication/request_handle.h"
#include "communication/serialization/en_decode_struct.h"
#include "communication/serialization/encode_response.h"

#include <signal.h>

static int is_servant_running = 1;

pid_t father = -1 ;

static int is_terminated = 0 ;

/**
 * @brief Signal handler to stop the servant gracefully
 */
static void servant_handle_signal(int sig) {
    (void)sig;
    is_servant_running = 0;
    is_terminated = 1 ;
}

static int proceed_simple(simple_request_t* req, int* fd_response){

    // Handling the request :
    void* ans = simple_request_handle(req, tasksdir);

    if(ans == NULL){
        write_log_msg("[daemon servant] Error : an error occured while handling a simple request");
        return -1;
    }

    if (daemon_open_reply(fd_response) < 0){
        write_log_msg("[daemon servant] Error : an error occured while opening the reply pipe");
        return -1;
    }

    int ret = 0;

    // Send the result :
    switch (req->opcode){
        case SE:
        case SO:
            ret = encode_a_output(*fd_response, (a_output_t *)ans);
            free_a_output((a_output_t *) ans);

            if(curr_output != NULL){
                free(curr_output);
                curr_output = NULL;
            }

            if(ret < 0){
                write_log_msg("[servant] Error encoding a_output answer");
            }
            break;

        case TX:
            ret = encode_a_timecode(*fd_response, (a_timecode_t *) ans);
            free_a_timecode((a_timecode_t *) ans);

            /*if(curr_time != NULL){
                time_array_free(curr_time);
                curr_time = NULL;
            }*/

            if(ret < 0){
                write_log_msg("[servant] Error encoding a_timecode answer");
            }
            break ;

        case LS:
            ret = encode_a_list(*fd_response, (a_list_t *) ans);
            free_a_list((a_list_t *) ans);

            if(ret < 0){
                write_log_msg("[servant] Error encoding a_list answer");
            }
            break;
        
        case TM:
            ret = encode_answer(*fd_response, (answer_t *) ans);
            free_answer((answer_t *) ans);
            is_terminated = 1;
            break;
        
        case RM:
            ret = encode_answer(*fd_response, (answer_t *) ans);
            free_answer((answer_t *) ans);

            if(ret < 0){
                write_log_msg("[servant] Error encoding answer");
            }
            else{
                if(((answer_t *) ans) -> anstype == OK){
                    // Notify the father that the tree-structure changed
                    kill(father, SIGUSR1);
                }
            }
            break;
        default:
            write_log_msg("[servant] unknown opcode %u", req->opcode);
            ret = -1;
            break;
    }
    return ret;
}

static int proceed_complex(complex_request_t* req, int* fd_response){

    answer_t* ans = complex_request_handle(req);

    if(ans == NULL){
        write_log_msg("[daemon servant] Error : an error occured while handling a complex request");
        return -1;
    }

    if (daemon_open_reply(fd_response) < 0){
        write_log_msg("[daemon servant] Error : an error occured while opening the reply pipe");
        return -1;
    }

    int ret = 0;

    switch (req->opcode){
        case CR:
        case CB:
            ret = encode_answer(*fd_response, ans);
            free_answer(ans);

            if(ret < 0){
                write_log_msg("[servant] Error encoding answer");
            }
            else{
                // Notify the father that the tree-structure changed
                kill(father, SIGUSR1);
            }
            break;
        default:
            write_log_msg("[servant] unknown opcode %u", req->opcode);
            ret = -1;
            break;
    }
    return ret;
}

static int proceed_request(int fd_request, int* fd_response){

    void* req = malloc(sizeof(complex_request_t));

    if(req == NULL){
        write_log_msg("[daemon servant] Error : an error occured while allocating memory for the request");
        return -1;
    }

    int val = daemon_read(&fd_request, req);

    if (val < 0) {
        write_log_msg("[daemon servant] Error while reading request");
        return -1;
    }

    int ret = 0;

    // Proceed based on the type of request
    // Simple :
    if(val == 1){
        simple_request_t* request = (simple_request_t*) req;

        write_log_msg("[daemon servant] Received simple request with opcode = %u", request->opcode);

        ret = proceed_simple(request, fd_response);

        free_simple_request(request);
    }
    // Complex :
    else{
        complex_request_t* request = (complex_request_t*) req;

        write_log_msg("[daemon servant] Received complex request with opcode = %u", request->opcode);

        ret = proceed_complex(request, fd_response);

        free_complex_request(request);
    }
    
    close(*fd_response);

    *fd_response = -1 ;
    return ret;
}

void start_serve(pid_t proc_father) {

    write_log_msg("Creation of the servant is a success");

    father = proc_father ;
    
    struct sigaction sa = {0};
    sa.sa_handler = servant_handle_signal;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    if (daemon_setup_pipes() < 0) {
        write_log_msg("[daemon servant] Error : failed to setup daemon pipes");
        return;
    }

    int fd_response = -1;

    write_log_msg("[daemon servant] Running start !");

    while(is_servant_running){

        if(is_terminated == 1){
            break ;
        }

        int fd_request = -1;

        write_log_msg("[daemon servant] Waiting for request...");

        if(proceed_request(fd_request, &fd_response) < 0){
            write_log_msg("[daemon servant] Error occured while proceeding the request");
            if (!is_servant_running) break;
            continue;
        }
        write_log_msg("[daemon servant] Sent OK");

        if(fd_request >= 0){
            close(fd_request);
        }
    }
    
    if (fd_response >= 0)
        close(fd_response);

    write_log_msg("[servant] stopped");

    // Cleanup before exiting
    if(curr_output){
        string_free(curr_output);
        curr_output = NULL;
    }

    if(curr_time){
        time_array_free(curr_time);
        curr_time = NULL;
    }

    write_log_msg("[servant] cleanup finish.");

    raise(SIGKILL); // Ensure termination just in case
}