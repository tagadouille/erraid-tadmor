#define _POSIX_C_SOURCE 200809L /* for PATH_MAX, etc. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "erraids/erraid.h"
#include "erraids/erraid-helper.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/**
 * @brief Create the default run directory for erraid and the pipes: /tmp/$USER/erraid and /tmp/$USER/erraid/pipes
*/
static void default_rundir(char *erraid_path, char* pipe_path, size_t err_size, size_t pipe_size) {

    const char *user = getenv("USER");
    if (!user) user = "nobody";

    snprintf(erraid_path, err_size, "/tmp/%s/erraid", user);
    snprintf(pipe_path, pipe_size, "/tmp/%s/pipes", user);

    dprintf(STDOUT_FILENO, "pipe path : %s", pipe_path);
}

int main(int argc, char **argv) {
    int opt;
    char rundir[PATH_MAX];
    char pipedir[PATH_MAX];

    default_rundir(rundir, pipedir, PATH_MAX, PATH_MAX);

    while ((opt = getopt(argc, argv, "R:FP:")) != -1) {

        switch (opt) {
            case 'R':
                // If it's valid -> copy the arguments in the pathes
                if (optarg && strlen(optarg) < sizeof(rundir)) {

                    strncpy(rundir, optarg, sizeof(rundir)-1);
                    strncpy(pipedir, optarg, sizeof(pipedir)-1);
                    rundir[sizeof(rundir)-1] = '\0';
                    pipedir[sizeof(rundir)-1] = '\0';
                }
                else {
                    dprintf(STDERR_FILENO, "Error : Invalid run directory\n");
                    return EXIT_FAILURE;
                }
                break;

            case 'F':
                //TODO foreground
                break;
            case 'P':
                if (optarg && strlen(optarg) < sizeof(pipedir)) {

                    strncpy(pipedir, optarg, sizeof(pipedir)-1);
                    pipedir[sizeof(pipedir)-1] = '\0';
                }
                else {
                    dprintf(STDERR_FILENO, "Error : Invalid pipe directory\n");
                    return EXIT_FAILURE;
                }
                break;
            default:
                dprintf(STDERR_FILENO, "Error : Invalid option gived\n");
                return EXIT_FAILURE;
        }
    }

    // Run directory for daemon
    if (erraid_set_rundir(rundir, pipedir) != 0) {
        dprintf(STDERR_FILENO, "Failed to set run directory '%s': %s\n", rundir, strerror(errno));
        return EXIT_FAILURE;
    }

    if (daemon_init() != 0) {
            dprintf(STDERR_FILENO, "daemon_init failed\n");
            return EXIT_FAILURE;
    }

    // Main run loop (blocks until signal)
    daemon_run();

    //Cleanup
    daemon_cleanup();

    return EXIT_SUCCESS;
}
