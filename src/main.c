#include <stdio.h>
#include "erraid.h"
#include <stdlib.h>
#include <stdint.h>

#include "tree-reading/tree_reader.h"

int main() {
    // initialize daemon
    if (daemon_init() != 0) {
        return EXIT_FAILURE;
    }

    // main loop
    daemon_run();

    // cleanup
    daemon_cleanup();

    // Run tests
    printf("Running tests...\n");

    printf("Test for listing\n");
    test_tree_reader(LIST);
    printf("Test for output\n\n");
    test_tree_reader(OUTPUT);
    printf("Test for err\n\n");
    test_tree_reader(ERR);
    printf("Test for time_exitcodes\n\n");
    test_tree_reader(TIME_EXIT);

    printf("All tests done.\n");
    return 0;
}

void test_tree_reader(Action_type action) {
    uint16_t task = 0;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR1 SUBDIR, task, action));
    printf("\n\n");

    task = 1;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR1 SUBDIR, task, action));
    printf("Test of task reader for more complex tasks\n");
    printf("\n\n");

    task = 4;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR2 SUBDIR, task, action));
    printf("\n\n");

    task = 15;
    printf("Test of task_reader for task %i \n Return value : %i\n", task, task_reader(TASKPATH DIR3 SUBDIR, task, action));
    printf("\n\n");
}
