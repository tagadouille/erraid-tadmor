#include "erraids/erraid.h"
#include "erraids/erraid-scanner.h"
#include "erraids/erraid-helper.h"

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


/* ---------------------------- CONFIG ---------------------------------- */

task_t* curr_task = NULL;

volatile int running = 1;


/**
 * The current log path
 */
char g_log_path[PATH_MAX] = "log";
/**
 * The current running directory
 */
char g_run_dir[PATH_MAX] = {0};

int g_log_fd = -1;
char tasksdir[PATH_MAX] = "";

string_t curr_output = {0};
time_array_t* curr_time = NULL;

/* --------------------------- SIGNAL HANDLER ---------------------------- */

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}
/* ---------------------------- DAEMON MODE ------------------------------ */

int daemon_init(void) {

    if (g_run_dir[0] == '\0') {
        const char *user = getenv("USER");
        if (!user) user = "nobody";
        snprintf(g_run_dir, sizeof(g_run_dir), "/tmp/%s/erraid", user);
    }

    if (ensure_rundir() != 0) return -1;

    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) _exit(EXIT_SUCCESS);

    if (setsid() < 0) return -1;

    pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) _exit(EXIT_SUCCESS);

    umask(0);
    if (chdir(g_run_dir) != 0) { fprintf(stderr, "chdir failed: %s\n", strerror(errno)); }

    g_log_fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (g_log_fd < 0) {
        fprintf(stderr, "Cannot open log file '%s': %s\n", g_log_path, strerror(errno));
        return -1;
    }

    dup2(g_log_fd, STDOUT_FILENO);
    dup2(g_log_fd, STDERR_FILENO);

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    write_log_msg("Daemon initialized (rundir=%s)", g_run_dir);
    return 0;
}

/* ------------------------------ MAIN LOOP ------------------------------ */

void daemon_run(void) {

    // Divide erraid : one scan the the task and the other execute the request
    switch (fork()){
        case -1:
            perror("fork");
            break;
        case 0:
            //TODO appeler le twin ✌️🥀💔
        default:
            // Scan of the task
            erraid_scan_loop();
            break;
    }
}