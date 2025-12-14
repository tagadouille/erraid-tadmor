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

/* ============================================================
 * CLIENT : ENVOYER UNE SIMPLE REQUÊTE ET LIRE UNE RÉPONSE
 * ============================================================ */
int client_send_simple( const char *rundir, const simple_request_t *req, answer_t *ans, int has_task){
    int fd_req, fd_rep;

    if (client_open_request(rundir, &fd_req) < 0)
        return -1;
    
    if (encode_simple_request(fd_req, req) < 0) {
        close(fd_req);
        return -1;
    }
    close(fd_req);

    if (client_open_reply(rundir, &fd_rep) < 0)
        return -1;

    if(ans == NULL){
        dprintf(STDERR_FILENO, "Error : The answer structure can't be null at client_send_simple\n");
        close(fd_rep);
        return -1;
    }

    if(ans->anstype == OK){

        if(has_task){
            if (decode_answer_ok_taskid(fd_rep, &(ans->task_id)) < 0) {
                dprintf(STDERR_FILENO, "Error : occured while decoding ok response with task id\n");
                close(fd_rep);
                return -1;
            }
        }
        else{
            if (decode_answer_ok_nopayload(fd_rep) < 0) {
                dprintf(STDERR_FILENO, "Error : occured while decoding ok response with no task id\n");
                close(fd_rep);
                return -1;
            }
        }
    }
    else if(ans->anstype == ERR){
        
        if (decode_answer_err(fd_rep, &(ans->errcode)) < 0) {
            dprintf(STDERR_FILENO, "Error : occured while decoding error response\n");
            close(fd_rep);
            return -1;
        }
    }
    else{
        dprintf(STDERR_FILENO, "Error : Unknow anstype at client_send_simple\n");
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
int daemon_reply_simple(const char *rundir, const answer_t *ans, int has_task)
{
    int fd_rep;

    if (daemon_open_reply(rundir, &fd_rep) < 0)
        return -1;

    if(ans == NULL){
        dprintf(STDERR_FILENO, "Error : The answer structure can't be null at daemon_reply_simple\n");
        close(fd_rep);
        return -1;
    }

    if(ans->anstype == OK){

        if(has_task){
            if (encode_answer_ok_taskid(fd_rep, ans->task_id) < 0) {
                dprintf(STDERR_FILENO, "Error : occured while encoding ok response with task id\n");
                close(fd_rep);
                return -1;
            }
        }
        else{
            if (encode_answer_ok_nopayload(fd_rep) < 0) {
                dprintf(STDERR_FILENO, "Error : occured while encoding ok response with no task id\n");
                close(fd_rep);
                return -1;
            }
        }
    }
    else if(ans->anstype == ERR){
        
        if (encode_answer_err(fd_rep, ans->errcode) < 0) {
                dprintf(STDERR_FILENO, "Error : occured while encoding error response\n");
                close(fd_rep);
                return -1;
            }
    }
    else{
        dprintf(STDERR_FILENO, "Error : Unknow anstype at daemon_reply_simple\n");
    }

    close(fd_rep);
    return 0;
}
