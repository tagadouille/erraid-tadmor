#include "erraids/erraid-log.h"
#include "erraids/erraid.h"
#include "tree-reading/tree_reader.h"
#include "erraids/erraid-executor.h"

#include <errno.h>
#include <stdint.h>
#include <time.h>

/**
 * All the tasks that was scanned
 */
static all_task_t *scanned_tasks = NULL;

/* ------------------------------ Timing util ---------------------------- */
/* wait until the next minute boundary (sleep until seconds == 0) */
static time_t wait_next_minute(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec = ts.tv_sec - (ts.tv_sec % 60) + 60;
    ts.tv_nsec = 0;

    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, NULL) == EINTR)
        ;

    return ts.tv_sec;
}

/**
 * @brief Scan all the task available in the tasksdir
 */
static void scan_all_task()
{

    write_log_msg("Scanning tasks directory…");

    scanned_tasks = all_task_listing(tasksdir);

    if (scanned_tasks == NULL)
    {
        write_log_msg("Error : an error occured while scanning all the tasks of path %s", tasksdir);
        running = 0;
        return;
    }
    write_log_msg("The scan succeded ! \n");
}

/**
 * @brief Execute all the task that was scanned
 * @param minute_now the actual minute
 */
static void execute_all_task(time_t minute_now)
{

    if (scanned_tasks == NULL)
    {
        write_log_msg("Error : scanned_tasks is NULL, can't execute all the tasks");
        return;
    }

    for (uint32_t i = 0; i < scanned_tasks->nbtask; i++)
    {
        // task_display(&(scanned_tasks -> all_task)[i]);
        run_task_if_due(&(scanned_tasks -> all_task)[i], minute_now);
    }

    write_log_msg("The execution is finish ! Go back to sleep.. zzz..\n");
}

void erraid_scan_loop()
{

    write_log_msg("Daemon main loop started.");
    // Scan of the tasks of the directory
    scan_all_task();

    // Execution loop of the tasks
    while (running)
    {

        time_t minute_now = wait_next_minute();

        execute_all_task(minute_now);
    }

    free(scanned_tasks);
    write_log_msg("Daemon main loop stopping.");
}