#include <stdio.h>
#include <stdlib.h>

#include "operation.h"
#include "test/test.h"

int main() {
    printf("Hello, World!\n");
    printf("Sum of 3 and 5 is: %d\n", sum(3, 5));
    printf("Multiply of 4 and 6 is: %d\n", multiply(4, 6));
    return 0;
}