#include "erraids/erraid-log.h"

static int is_servant_running = 1;

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
        simple_request_t req;

        if (decode_simple_request(fd_request, &req) < 0) {
            perror("decode_simple_request");
            _exit(1);
        }

        write_log_msg("[daemon] Received opcode = %u", req.opcode);

        //TODO exécuter la requête puis y répondre

        /* --- open reply pipe --- */
        int fd_rep;
        if (daemon_open_reply(rundir, &fd_response) < 0) {
            perror("daemon_open_reply");
            _exit(1);
        }

        /* --- send OK (no payload) --- */
        if (encode_answer_ok_nopayload(fd_response) < 0) {
            perror("encode_answer_ok_nopayload");
            _exit(1);
        }

        write_log_msg("[daemon servant] Sent OK");
    }
    close(fd_response);
    close(fd_request);
}