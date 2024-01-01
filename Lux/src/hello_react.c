// hello_react.c

#include <assert.h>
#include <stdio.h>

void hello_react() {
    printf("Hello, React!\n");
}

void process_data(double* input, double* output, int size) {
    int i;

    assert(size > 0 && "size must be positive");
    assert(input && output && "must be valid pointers");

    for (i = 0; i < size; i++) {
        output[i] = input[i] * input[i];
    }
}