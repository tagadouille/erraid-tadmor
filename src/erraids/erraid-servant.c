#include "erraids/erraid-log.h"
#include "erraids/erraid.h"
#include "communication/answer.h"
#include "communication/request.h"
#include "communication/pipes.h"
#include "communication/communication.h"
#include "communication/request_handle.h"
#include "communication/serialization/en_decode_struct.h"
#include "communication/serialization/encode_response.h"

static int is_servant_running = 1;

static int proceed_request(simple_request_t* req, int fd_request, int fd_response){

    if (daemon_read_simple(fd_request, req) < 0) {
        perror("decode_simple_request");
        _exit(1);
    }

    write_log_msg("[daemon servant] Received opcode = %u", req->opcode);

    void* ans = simple_request_handle(req, tasksdir);

    if(ans == NULL){
        write_log_msg("[daemon servant] Error : an error occured while handling a simple request");
        return -1;
    }

    if (daemon_open_reply(&fd_response) < 0){
        write_log_msg("[daemon servant] Error : an error occured while opening the reply pipe");
        return -1;
    }

    switch (req->opcode){
        case SE:
        case SO:

            if (encode_a_output(fd_response, (a_output_t *) ans) < 0) {
                write_log_msg("[daemon servant] Error : an error occured while encode a_output");
                close(fd_response);
                return -1;
            }
            close(fd_response);
            return 0;

        case TX:

            if (encode_a_timecode(fd_response, (a_timecode_t *) ans) < 0) {
                write_log_msg("[daemon servant] Error : an error occured while encode a_timecode");
                close(fd_response);
                return -1;
            }
            close(fd_response);
            return 0;

        case LS:
             
            if (encode_a_timecode(fd_response, (a_timecode_t *) ans) < 0) {
                write_log_msg("[daemon servant] Error : an error occured while encode a_timecode");
                close(fd_response);
                return -1;
            }
            close(fd_response);
            return 0;
    }
    close(fd_response);

    if (daemon_reply_simple((answer_t *) ans) < 0) {
        perror("daemon_open_reply");
        _exit(1);
    }
    return 0;
}

void start_serve(){

    write_log_msg("Creation of the twin ✌️🥀💔 is a succes");

    int fd_request;
    if (daemon_setup_pipes(&fd_request) < 0) {
        write_log_msg("[daemon servant] Error : failed to setup daemon pipes");
        _exit(1);
    }
    int fd_response;

    write_log_msg("[daemon servant] Running start !");

    while(is_servant_running){

        write_log_msg("[daemon servant] Waiting for simple request...");
        simple_request_t* req = NULL;

        if(proceed_request(req, fd_request, fd_response) < 0){
            write_log_msg("[daemon servant] Error occured while proceeding the request");
            _exit(1);
        }
        write_log_msg("[daemon servant] Sent OK");
    }
    close(fd_response);
    close(fd_request);
}