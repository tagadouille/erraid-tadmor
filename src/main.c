#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "operation.h"
#include "test/test.h"
#include "tree-reading/tree_reader.h"

int main() {
    printf("Hello, World!\n");
    printf("Sum of 3 and 5 is: %d\n", sum(3, 5));
    printf("Multiply of 4 and 6 is: %d\n", multiply(4, 6));

    printf("Running tests...\n");
    uint16_t task = 0;
    printf("Test of task_reader for task %i \n", task);
    int result = task_reader(TASKPATH DIR1 SUBDIR, task);
    printf("Test of task_reader for task %i \n Return value : %i\n", task, result);
    return 0;
}