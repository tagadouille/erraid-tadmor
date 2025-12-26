#include "erraids/erraid-log.h"
#include "erraids/erraid.h"
#include "tree-reading/tree_reader.h"
#include "erraids/erraid-executor.h"

#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

/**
 * All the tasks that was scanned
 */
static all_task_t* scanned_tasks = NULL;

/**
 * If rescan is needed or not
 */
static volatile sig_atomic_t need_rescan = 0;

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

/**
 * @brief Scan all the task available in the tasksdir
 */
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
 * @param minute_now the actual minute 
 */
static void execute_all_task(time_t minute_now){

    if(scanned_tasks == NULL){
        write_log_msg("Error : scanned_tasks is NULL, can't execute all the tasks");
        return;
    }

    for (uint32_t i = 0; i < scanned_tasks -> nbtask; i++)
    {
        run_task_if_due(&(scanned_tasks -> all_task)[i], minute_now);
    }

    write_log_msg("The execution is finish ! Go back to sleep.. zzz..\n");
}

/**
 * @brief Use for sigaction for rescan the tree-structure when
 * erraid servant changed it
 */
static void rescan(int sig) {
    (void)sig;
    need_rescan = 1;
}

/**
 * @brief Define the behavior when SIGUSR1 is sent by
 * erraid servant
 */
static void setup_signal_handler(void) {
    struct sigaction sa;
    
    sa.sa_handler = rescan;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; //! Important: restart interrupted syscall
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    // Put at default after sigaction
    signal(SIGUSR1, SIG_DFL); 
}

void erraid_scan_loop(void) {
    write_log_msg("Daemon main loop started.");
    
    setup_signal_handler();
    write_log_msg("Daemon PID: %d", getpid());
    
    scan_all_task();
    
    while (running) {
        // Wait next minute
        time_t scheduled_time = wait_next_minute();
        
        // 2. Check if after the end of the sleep a rescan have been requested
        if (need_rescan) {
            write_log_msg("Rescan requested, scanning tasks");
            scan_all_task();
            need_rescan = 0;
            
            // After a rescan, verify if the tasks must be executed
            time_t now = time(NULL);
            if (now >= scheduled_time) {
                // On a dépassé le temps prévu, exécuter maintenant
                time_t current_minute = now - (now % 60);
                write_log_msg("Executing missed minute: %ld", current_minute);
                execute_all_task(current_minute);
            }
            // Continue the loop for the next minute
            continue;
        }
        
        //Execute task at the exact minute
        execute_all_task(scheduled_time);
    }
    
    free(scanned_tasks);
    write_log_msg("Daemon main loop stopping.");
}