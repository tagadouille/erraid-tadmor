#define _POSIX_C_SOURCE 200809L
#include "communication/communication.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/encode_request.h"
#include "communication/serialization/decode_request.h"
#include "communication/serialization/decode_response.h"
#include "communication/serialization/encode_response.h"
#include "communication/pipes.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

char pipe_path[PATH_MAX] = {0};

/* ============================================================
 * CLIENT : ENVOYER UNE SIMPLE REQUÊTE ET LIRE UNE RÉPONSE
 * ============================================================ */
int client_send_simple(const simple_request_t *req, int *fd_rep_out){
    int fd_req, fd_rep;

    if (client_open_request(&fd_req) < 0) {
        dprintf(STDERR_FILENO, "Error : occured while opening request pipe in client_send_simple\n");
        return -1;
    }

    if (encode_simple_request(fd_req, req) < 0) {
        close(fd_req);
        dprintf(STDERR_FILENO, "Error : occured while encoding request in client_send_simple\n");
        return -1;
    }
    close(fd_req);

    if (client_open_reply(&fd_rep) < 0) {
        dprintf(STDERR_FILENO, "Error : occured while opening reply pipe in client_send_simple\n");
        return -1;
    }

    *fd_rep_out = fd_rep;
    return 0;
}

// receive answer from daemon and decode it based on opcode
int client_recv_answer(uint16_t opcode, int fd_rep){
    int ret = 0;

    switch (opcode) {

        case LS: { 
            a_list_t ans = {0};
            ret = decode_a_list(fd_rep, &ans);
            dprintf(STDOUT_FILENO, "Received %u tasks\n", ans.all_task.nbtask);
            if (ret <0){
                dprintf(STDERR_FILENO, "Error decoding a_list answer\n");
                return -1;
            }
            break;
        }

        case SO:
        case SE:{
            a_output_t out = {0};
            ret = decode_a_output(fd_rep, &out);
            break;
        }

        case TX:{
            a_timecode_t tc = {0};
            ret = decode_a_timecode(fd_rep, &tc);
            break;
        }

        case CR:
        case RM:
        case CB:
        case TM: { 
            answer_t ans = {0};
            ret = decode_answer(fd_rep, &ans);
            break;
        }

        default:
            dprintf(2, "Unknown opcode %u\n", opcode);
            ret = -1;
    }
    dprintf(STDOUT_FILENO, "Answer received for opcode %u with %u\n", opcode, ret);
    close(fd_rep);
    return ret;
}


/* ============================================================
 * DEMON : LIRE UNE SIMPLE REQUÊTE
 * ============================================================ */
int daemon_read_simple(int fd_req, simple_request_t *req){
    return decode_simple_request(fd_req, req);
}

/* ============================================================
 * DEMON : ENVOYER UNE SIMPLE RÉPONSE
 * ============================================================ */
int daemon_reply_simple(const answer_t *ans)
{
    int fd_rep;

    if (daemon_open_reply(&fd_rep) < 0)
        return -1;

    if (encode_answer(fd_rep, ans) < 0) {
        close(fd_rep);
        return -1;
    }

    close(fd_rep);
    return 0;
}
