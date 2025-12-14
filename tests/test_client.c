#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "communication/pipes.h"
#include "communication/request.h"
#include "communication/answer.h"
#include "serialization.h"
#include "communication/communication.h"

int main(void)
{
    const char *rundir = "/tmp/test-erraid";

    simple_request_t req = {
        .opcode = LS
    };

    answer_t ans;

    printf("[CLIENT] Sending LS request…\n");

    if (client_send_simple(rundir, &req, &ans) < 0) {
        perror("client_send_simple");
        exit(1);
    }

    printf("[CLIENT] Response received! anstype=%04x\n", ans.anstype);

    return 0;
}
