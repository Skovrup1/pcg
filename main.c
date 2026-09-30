#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.c"
#include "core.h"
#include "thread_pool.c"

#define VECTOR_WIDTH 8 // elements

void print_array(S32 *data, S64 count) {
    for (int i = 0; i < count; i++) {
        printf("%2d ", data[i]);
    }
    printf("\n");
}

Bool verify(S32 *input, S32 *output, S64 count) {
    S32 *expected = calloc(count, sizeof(S32));

    S32 acc = 0;
    for (int i = 0; i < count; i++) {
        expected[i] = acc;
        acc += input[i];
    }

    S32 is_match = memcmp(expected, output, count * sizeof(S32));

    print_array(expected, count);
    free(expected);

    if (is_match == 0) {
        return true;
    } else {
        return false;
    }
}

U64 start_time = 0;
U64 end_time = 0;

int main() {
    const int count = 1 << 20; // 1 million elements
    const int num_threads = 4;
    const int num_runs = 10;

    S32 *input = malloc(sizeof(S32) * count);
    S32 *output = malloc(sizeof(S32) * count);

    for (int i = 0; i < count; i++) {
        input[i] = i + 1;
    }

    for (int i = 0; i < num_runs; i++) {
        start_time += read_cpu_timer();
        prefix_sum_blelloch_multithreaded(input, output, count, num_threads);
        end_time += read_cpu_timer();
    }

    Bool is_valid = verify(input, output, count);

    if (is_valid == true) {
        printf("valid\n");
    } else {
        printf("not valid\n");
    }

    // print_array(output, count);

    U64 cycles_elapsed = (end_time - start_time) / num_runs;
    printf("cycles elapsed = %lu\n", cycles_elapsed);

    free(input);
    free(output);

    return 0;
}


