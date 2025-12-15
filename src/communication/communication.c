#define _POSIX_C_SOURCE 200809L
#include "communication/communication.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/encode_request.h"
#include "communication/serialization/decode_request.h"
#include "communication/serialization/decode_response.h"
#include "communication/serialization/encode_response.h"
#include "communication/pipes.h"

#include <unistd.h>
#include <errno.h>

char pipe_path[PATH_MAX] = {0};

/* ============================================================
 * CLIENT : ENVOYER UNE SIMPLE REQUÊTE ET LIRE UNE RÉPONSE
 * ============================================================ */
int client_send_simple(const simple_request_t *req, answer_t *ans, int has_task){
    int fd_req, fd_rep;

    if (client_open_request(&fd_req) < 0)
        return -1;
    
    if (encode_simple_request(fd_req, req) < 0) {
        close(fd_req);
        return -1;
    }
    close(fd_req);

    if (client_open_reply(&fd_rep) < 0)
        return -1;

    if (decode_answer(fd_rep, ans) < 0) {
        dprintf(STDERR_FILENO, "Error : occured while decoding answer in client_send_simple\n");
        close(fd_rep);
        return -1;
    }

    close(fd_rep);
    return 0;
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
int daemon_reply_simple(const answer_t *ans, int has_task)
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
