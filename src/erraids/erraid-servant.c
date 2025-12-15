#include "erraids/erraid-log.h"
#include "communication/answer.h"
#include "communication/request.h"

static int is_servant_running = 1;

static void* proceed_request(simple_request_t* req){

    if (decode_simple_request(fd_request, req) < 0) {
        perror("decode_simple_request");
        _exit(1);
    }

    write_log_msg("[daemon] Received opcode = %u", req->opcode);

    //TODO convertir la requête
    return simple_request_handle(req, tasksdir);
}

void start_serve(){

    write_log_msg("Creation of the twin ✌️🥀💔 is a succes");

    //TODO ouvrir les tubes et attendre les requêtes et y répondre
    int fd_request;
    if (daemon_setup_pipes(rundir, &fd_req) < 0) {
        write_log_msg("Error : failed to setup daemon pipes");
        _exit(1);
    }
    int fd_response;

    write_log_msg("Running start !");
    while(servant_running){

        write_log_msg("[daemon servant] Waiting for simple request...");
        simple_request_t* req = NULL;
        answer_t* answer = NULL;

        proceed_request(req, ans);

        //TODO exécuter la requête puis y répondre

        /* --- open reply pipe --- */
        int fd_rep;
        if (daemon_reply_simple(const answer_t *ans, int has_task) < 0) {
            perror("daemon_open_reply");
            _exit(1);
        }

        write_log_msg("[daemon servant] Sent OK");
    }
    close(fd_response);
    close(fd_request);
}