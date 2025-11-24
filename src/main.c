#define _POSIX_C_SOURCE 200809L /* for PATH_MAX, etc. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "erraid.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Default run directory: /tmp/$USER/erraid */
static void default_rundir(char *out, size_t n) {
    const char *user = getenv("USER");
    if (!user) user = "nobody";
    snprintf(out, n, "/tmp/%s/erraid", user);
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [-r RUN_DIR] [-f]\n"
        "  -r RUN_DIR   : root run directory (default /tmp/$USER/erraid)\n"
        "  -f           : foreground (do not daemonize) - useful for debug\n",
        prog);
}

int main(int argc, char **argv) {
    int opt;
    char rundir[PATH_MAX];
    int foreground = 0;

    default_rundir(rundir, sizeof(rundir));

    while ((opt = getopt(argc, argv, "r:fh")) != -1) {
        switch (opt) {
            case 'r':
                if (optarg && strlen(optarg) < sizeof(rundir)) {
                    strncpy(rundir, optarg, sizeof(rundir)-1);
                    rundir[sizeof(rundir)-1] = '\0';
                } else {
                    fprintf(stderr, "Invalid run directory\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'f':
                foreground = 1;
                break;
            case 'h':
            default:
                usage(argv[0]);
                return (opt == 'h') ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    /* set run directory for daemon */
    if (erraid_set_rundir(rundir) != 0) {
        fprintf(stderr, "Failed to set run directory '%s': %s\n", rundir, strerror(errno));
        return EXIT_FAILURE;
    }

    if (!foreground) {
        if (daemon_init() != 0) {
            fprintf(stderr, "daemon_init failed\n");
            return EXIT_FAILURE;
        }
    } else {
        /* In foreground mode we still initialize resources but skip forking */
        if (erraid_init_foreground() != 0) {
            fprintf(stderr, "foreground init failed\n");
            return EXIT_FAILURE;
        }
    }

    /* Main run loop (blocks until signal) */
    daemon_run();

    /* Cleanup */
    daemon_cleanup();

    return EXIT_SUCCESS;
}
