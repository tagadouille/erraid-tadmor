#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include "tadmor.h"
#include "communication/answer.h"
//#include "communication/request.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* --------------------------------------------------------------
 * default rundir: /tmp/$USER/tadmor
 * -------------------------------------------------------------- */
static void default_rundir(char *buf, size_t n)
{
    const char *user = getenv("USER");
    if (!user) user = "nobody";
    snprintf(buf, n, "/tmp/%s/tadmor", user);
}

/* --------------------------------------------------------------
 * Usage
 * -------------------------------------------------------------- */
static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [-r RUN_DIR] <command-line>\n\n"
        "Examples:\n"
        "   tadmor ls\n"
        "   tadmor rm <id>\n"
        "   tadmor stdout <id>\n"
        "   tadmor stderr <id>\n"
        "   tadmor times <id>\n"
        "   tadmor terminate\n",
        prog);
}

/* --------------------------------------------------------------
 * Mini-parser: take the full command line and dispatch it
 * -------------------------------------------------------------- */
int client_handle_command(const char *input)
{
    char cmd[64];
    uint64_t id = 0;

    /* Try to extract a command and optionally a number (id) */
    int n = sscanf(input, "%63s %lu", cmd, &id);

    if (n <= 0) {
        fprintf(stderr, "Invalid command\n");
        return -1;
    }

    /* ---------------------- NO-ARG COMMANDS ---------------------- */

    if (strcmp(cmd, "ls") == 0) {
        a_list_t *ans = client_ls();
        if (!ans) { fprintf(stderr, "ls: no answer\n"); return -1; }
        printf("[ls] nbtask = %u\n", ans->nbtask);
        free_a_list(ans);
        return 0;
    }

    if (strcmp(cmd, "terminate") == 0) {
        answer_t *ans = client_terminate();
        if (!ans) { fprintf(stderr, "terminate: no answer\n"); return -1; }
        printf("[terminate] anstype = 0x%x\n", ans->anstype);
        free_answer(ans);
        return 0;
    }

    /* ---------------------- COMMANDS WITH ID --------------------- */

    if (n < 2) {
        fprintf(stderr, "%s: missing <id>\n", cmd);
        return -1;
    }

    if (strcmp(cmd, "rm") == 0) {
        answer_t *ans = client_rm(id);
        if (!ans) { fprintf(stderr, "rm: no answer\n"); return -1; }
        printf("[rm] anstype = 0x%x\n", ans->anstype);
        free_answer(ans);
        return 0;
    }

    if (strcmp(cmd, "stdout") == 0) {
        a_output_t *ans = client_stdout(id);
        if (!ans) { fprintf(stderr, "stdout: no answer\n"); return -1; }
        printf("[stdout] anstype = 0x%x\n", ans->anstype);
        free_a_output_t(ans);
        return 0;
    }

    if (strcmp(cmd, "stderr") == 0) {
        a_output_t *ans = client_stderr(id);
        if (!ans) { fprintf(stderr, "stderr: no answer\n"); return -1; }
        printf("[stderr] anstype = 0x%x\n", ans->anstype);
        free_a_output_t(ans);
        return 0;
    }

    if (strcmp(cmd, "times") == 0) {
        a_timecode_t *ans = client_times(id);
        if (!ans) { fprintf(stderr, "times: no answer\n"); return -1; }
        printf("[times] nbrun = %u\n", ans->nbrun);
        free_a_timecode_t(ans);
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    return -1;
}

/* --------------------------------------------------------------
 * main
 * -------------------------------------------------------------- */
int main(int argc, char **argv)
{
    int opt;
    char rundir[PATH_MAX];

    default_rundir(rundir, sizeof(rundir));

    /* Parse -r */
    while ((opt = getopt(argc, argv, "r:h")) != -1) {
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

        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;

        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* After options, we expect a command line */
    if (optind >= argc) {
        fprintf(stderr, "Error: no command provided.\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Reconstruct the remaining arguments into one string */
    char input[512] = {0};
    size_t pos = 0;

    for (int i = optind; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (pos + len + 2 >= sizeof(input)) break;
        memcpy(input + pos, argv[i], len);
        pos += len;
        input[pos++] = ' ';
    }
    input[pos] = '\0';

    /* Prepare client */
    if (client_set_rundir(rundir) != 0) {
        fprintf(stderr, "client_set_rundir failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (client_connect() != 0) {
        fprintf(stderr, "client_connect failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    /* Handle the command with your mini parser */
    int rc = client_handle_command(input);

    client_disconnect();
    return (rc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}