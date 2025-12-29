#define _POSIX_C_SOURCE 200809L

#include "communication/communication.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/encode_request.h"
#include "communication/serialization/decode_request.h"
#include "communication/serialization/decode_response.h"
#include "communication/serialization/encode_response.h"
#include "communication/pipes.h"
#include "tree-reading/tree_reader.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

char pipe_path[PATH_MAX] = {0};

/* ==============================
 * CLIENT : SEND A SIMPLE REQUEST
 * ==============================*/
int client_send_simple(const simple_request_t *req)
{
    int fd_req;

    if (client_open_request(&fd_req) < 0) {
        dprintf(STDERR_FILENO,
                "Error: opening request pipe in client_send_simple\n");
        return -1;
    }

    if (encode_simple_request(fd_req, req) < 0) {
        dprintf(STDERR_FILENO,
                "Error: encoding request in client_send_simple\n");
        close(fd_req);
        return -1;
    }

    /* IMPORTANT : fermeture = fin de message */
    close(fd_req);
    return 0;
}

/* =======================
 * CLIENT : GET A RESPONSE
 * ======================= */
void* client_recv_answer(uint16_t opcode)
{
    int fd_rep;
    void* ret = NULL;

    if (client_open_reply(&fd_rep) < 0) {
        dprintf(STDERR_FILENO, "Error: opening reply pipe in client_recv_answer\n");
        return ret;
    }

    switch (opcode) {

        case LS: {
            ret = decode_a_list(fd_rep);
            if(ret == NULL) dprintf(STDERR_FILENO, "Error : an error occured while decoding a_list\n");
            break;
        }

        case SO:
        case SE: {
            ret = decode_a_output(fd_rep);
            if(ret == NULL) dprintf(STDERR_FILENO, "Error : an error occured while decoding a_output\n");
            break;
        }

        case TX: {
            ret = decode_a_timecode(fd_rep);
            if(ret == NULL) dprintf(STDERR_FILENO, "Error : an error occured while decoding a_timecode\n");
            break;
        }

        case CR:
        case RM:
            ret = decode_answer(fd_rep);
            if(ret == NULL) dprintf(STDERR_FILENO, "Error : an error occured while decoding a_answer\n");
            break;
        case CB:
        case TM: {
            ret = decode_answer(fd_rep);
            if(ret == NULL) dprintf(STDERR_FILENO, "Error : an error occured while decoding answer_t\n");
            break;
        }

        default:
            dprintf(STDERR_FILENO,"Unknown opcode %u\n", opcode);
    }

    close(fd_rep);
    return ret;
}

/* ==============================
 * DAEMON : READ A SIMPLE REQUEST
 * ============================== */
int daemon_read_simple(int* fd_req, simple_request_t *req){

    if(pipe_file_read() < 0){
        dprintf(2, "Error : an error occured while reading the pipe_file\n");
        return -1;
    }
    
    if(daemon_setup_pipes() < 0){
        dprintf(2, "Error : an error occured while reading the pipe_file\n");
        return -1;
    }

    char *req_path = make_path_no_test(pipe_path, REQUEST_PIPE);

    if(req_path == NULL){
        dprintf(STDERR_FILENO, "Error : an error occured while creating the path to the request pipe\n");
        return -1;
    }

    int r = open(req_path, O_RDWR);
    free(req_path);

    if (r < 0){
        perror("open");
        return -1;
    }
        
    *fd_req = r;
    /* daemon read until EOF (client close the pipe) */
    return decode_simple_request(*fd_req, req);
}

/* ================================
 * DAEMON : SEND A SIMPLE RESPONSE
 * ================================ */
int daemon_reply_simple(const answer_t *ans)
{
    int fd_rep;

    if (daemon_open_reply(&fd_rep) < 0) {
        dprintf(STDERR_FILENO,
                "Error: opening reply pipe in daemon_reply_simple\n");
        return -1;
    }

    if (encode_answer(fd_rep, ans) < 0) {
        dprintf(STDERR_FILENO,
                "Error: encoding answer in daemon_reply_simple\n");
        close(fd_rep);
        return -1;
    }

    close(fd_rep);
    return 0;
}
