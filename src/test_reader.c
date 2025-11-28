#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include "tree-reading/tree_reader.h"
#include "types/timing.h"
#include "types/task.h"
#include "erraid.h"

void test_tree_reader(Action_type action);

int main() {
    printf("Running tests...\n\n");

    printf("Test for listing\n");
    test_tree_reader(LIST);
    printf("Test for output\n\n");
    test_tree_reader(OUTPUT);
    printf("Test for err\n\n");
    test_tree_reader(ERR);
    printf("Test for time_exitcodes\n\n");
    test_tree_reader(TIME_EXIT);
    task_reader(TASKPATH DIR2 SUBDIR, 4, LIST);

    for (size_t i = 0; i < 70; i++)
    {
        sleep(1);
        if(timing_should_run(curr_task -> timing)){
            dprintf(STDOUT_FILENO, "La TAAAACHE A ETE ACTIIIIIVER\n");
            break;
        }else{
            dprintf(STDOUT_FILENO, "j'attends mon activation\n");
        }
    }
    
    printf("All tests done.\n");
    return 0;
}

void test_tree_reader(Action_type action) {
    uint16_t task = 0;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR1 SUBDIR, task, action));
    
    if(curr_task != NULL){
        task_display(curr_task);
        task_destroy(curr_task);
        curr_task = NULL;
    }
    printf("\n\n");

    task = 1;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR1 SUBDIR, task, action));
    printf("Test of task reader for more complex tasks\n");

    if(curr_task != NULL){
        task_display(curr_task);
        task_destroy(curr_task);
        curr_task = NULL;
    }
    printf("\n\n");

    task = 4;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR2 SUBDIR, task, action));

    if(curr_task != NULL){
        task_display(curr_task);
        task_destroy(curr_task);
        curr_task = NULL;
    }
    printf("\n\n");

    task = 15;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR3 SUBDIR, task, action));

    if(curr_task != NULL){
        task_display(curr_task);
        task_destroy(curr_task);
        curr_task = NULL;
    }
    printf("\n\n");
}