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
static all_task_t *scanned_tasks = NULL;

/**
 * If rescan is needed or not
 */
static volatile sig_atomic_t need_rescan = 0;

/**
 * Last minute executed (to avoid double execution)
 */
static time_t last_executed_minute = 0;


/**
 * @brief Wait until the next minute starting with :00
 * @return The exact minute timestamp ending with :00
 * 
 * If interrupted by SIGUSR1 with need_rescan set, returns current time immediately.
 */
static time_t wait_next_minute(void)
{
    struct timespec ts;
    
    // Get current time
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("clock_gettime");
        return time(NULL);
    }
    
    // Calculate next minute :00
    time_t next_minute = ts.tv_sec - (ts.tv_sec % 60) + 60;
    
    // Prepare absolute time for sleep
    struct timespec target = {
        .tv_sec = next_minute,
        .tv_nsec = 0
    };
    
    // Sleep until next minute :00
    while (1) {
        int result = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target, NULL);
        
        // Full sleep completed
        if (result == 0) {
            return next_minute;
        }
        // Interrupted by signal
        else if (result == EINTR) {
            
            // Check if we need to stop
            return time(NULL);
        } 
        else {
            perror("clock_nanosleep");
            return time(NULL);
        }
    }
}

/**
 * @brief Scan all the task available in the tasksdir
 */
static void scan_all_task(void) {

    write_log_msg("Scanning tasks directory…");

    if (scanned_tasks != NULL) {
        free_all_task(scanned_tasks);
    }

    scanned_tasks = all_task_listing(tasksdir);
    
    if (scanned_tasks == NULL) {
        write_log_msg("Error: an error occurred while scanning all the tasks of path %s", tasksdir);
        running = 0;
        return;
    }
    
    write_log_msg("Scan succeeded! Found %u tasks\n", scanned_tasks->nbtask);
}

/**
 * @brief Execute all the task that was scanned
 * @param minute_now the actual minute (must end with :00)
 */
static void execute_all_task(time_t minute_now) {

    if (scanned_tasks == NULL) {
        write_log_msg("Error: scanned_tasks is NULL, can't execute tasks");
        return;
    }
    
    // Ensure minute_now is at :00 boundary
    if (minute_now % 60 != 0) {
        minute_now = minute_now - (minute_now % 60);
    }
    
    // Avoid executing the same minute twice
    if (minute_now == last_executed_minute) {
        write_log_msg("Skipping already executed minute");
        return;
    }

    // Execution :
    write_log_msg("Executing tasks");
    
    for (uint32_t i = 0; i < scanned_tasks->nbtask; i++) {
        run_task_if_due(&(scanned_tasks->all_task)[i], minute_now);
    }
    
    last_executed_minute = minute_now;
    write_log_msg("Execution finished for minute\n");
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
    sa.sa_flags = 0;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Daemon main loop
 */
void erraid_scan_loop(void) {
    
    write_log_msg("Daemon main loop started.");
    
    setup_signal_handler();
    write_log_msg("Daemon PID: %d", getpid());
    
    // Initial scan
    scan_all_task();
    
    // Main loop
    while (running) {
        time_t current_time = time(NULL);
        write_log_msg("Current time: %ld (%ld seconds until next minute)", current_time, 60 - (current_time % 60));
        
        // Wait for next minute :00
        time_t wake_time = wait_next_minute();
        
        write_log_msg("Woke up");
        
        // Handle rescan if requested
        if (need_rescan) {
            write_log_msg("Rescan requested by signal");
            scan_all_task();
            need_rescan = 0;
            
            // Check if we missed an execution during rescan
            time_t now = time(NULL);
            time_t current_minute = now - (now % 60);
            
            // If we're at exact minute :00 and haven't executed it yet
            if (now % 60 == 0 && current_minute != last_executed_minute) {
                write_log_msg("Executing missed minute after rescan: %ld", current_minute);
                execute_all_task(current_minute);
            }
            
            // Continue to wait for next minute
            continue;
        }

        // Execute tasks for this minute
        execute_all_task(wake_time);
    }

    if (scanned_tasks != NULL) {
        free_all_task(scanned_tasks);
        scanned_tasks = NULL;
    }
    
    write_log_msg("Daemon main loop stopping.");
}