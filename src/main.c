#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>


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