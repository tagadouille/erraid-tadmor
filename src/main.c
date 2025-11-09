#include <stdio.h>
#include "erraid.h"

int main() {
    // initialize daemon
    if (daemon_init() != 0) {
        return EXIT_FAILURE;
    }

    // main loop
    daemon_run();

    // cleanup
    daemon_cleanup();

    return EXIT_SUCCESS;
}
