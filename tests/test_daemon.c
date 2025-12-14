#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "communication/pipes.h"
#include "serialization.h"
#include "communication/request.h"

int main() {

    const char *rundir = "run_test";
    mkdir(rundir, 0777);

    int fd_req;
    if (daemon_setup_pipes(rundir, &fd_req) < 0) {
        perror("daemon_setup_pipes");
        exit(1);
    }

    printf("[daemon] Waiting for simple request...\n");

    /* --- read simple request --- */
    simple_request_t req;
    if (decode_simple_request(fd_req, &req) < 0) {
        perror("decode_simple_request");
        exit(1);
    }

    printf("[daemon] Received opcode = %u\n", req.opcode);

    /* --- open reply pipe --- */
    int fd_rep;
    if (daemon_open_reply(rundir, &fd_rep) < 0) {
        perror("daemon_open_reply");
        exit(1);
    }

    /* --- send OK (no payload) --- */
    if (encode_answer_ok_nopayload(fd_rep) < 0) {
        perror("encode_answer_ok_nopayload");
        exit(1);
    }

    printf("[daemon] Sent OK\n");

    close(fd_rep);
    close(fd_req);
    return 0;
}
