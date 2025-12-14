#include "erraids/erraid.h"
#include "erraids/erraid-helper.h"
#include "erraids/erraid-log.h"
#include "erraids/erraid-executor.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

#include <stdarg.h>
#include <dirent.h>
#include <sys/file.h>

#include "types/task.h"
#include "tree-reading/tree_reader.h"
#include "test.h"


/* ---------------------------- CONFIG ---------------------------------- */

static volatile int running = 1;

/**
 * All the tasks that was scanned
 */
static all_task_t* scanned_tasks = NULL;


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


/* ------------------------------ Timing util ---------------------------- */
/* wait until the next minute boundary (sleep until seconds == 0) */
static time_t wait_next_minute(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec = ts.tv_sec - (ts.tv_sec % 60) + 60;
    ts.tv_nsec = 0;

    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, NULL) == EINTR);

    return ts.tv_sec;
}

/* ------------------------------ MAIN LOOP ------------------------------ */

static void scan_all_task(){

    write_log_msg("Scanning tasks directory…");

    scanned_tasks = all_task_listing(tasksdir);

    if(scanned_tasks == NULL){
        write_log_msg("Error : an error occured while scanning all the tasks of path %s", tasksdir);
        running = 0;
        return;
    }
    write_log_msg("The scan succeded ! \n");
}

/**
 * @brief Execute all the task that was scanned
 */
static void execute_all_task(time_t minute_now){

    if(scanned_tasks == NULL){
        write_log_msg("Error : scanned_tasks is NULL, can't execute all the tasks");
        return;
    }

    for (uint32_t i = 0; i < scanned_tasks -> nbtask; i++)
    {
        //task_display(&(scanned_tasks -> all_task)[i]);
        run_task_if_due(&(scanned_tasks -> all_task)[i], minute_now);
    }

    write_log_msg("The execution is finish ! Go back to sleep.. zzz..\n");
    
}

void daemon_run(void) {
    write_log_msg("Daemon main loop started.");
    
    // Scan of the tasks of the directory
    scan_all_task();

    // Execution loop of the tasks
    while (running) {

        time_t minute_now = wait_next_minute();

        execute_all_task(minute_now);
    }

    free(scanned_tasks);
    write_log_msg("Daemon main loop stopping.");
}