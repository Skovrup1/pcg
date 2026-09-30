#include "core.h"
#include "thread_pool.h"
#include <immintrin.h>
#include <math.h>
#include <string.h>
#include <x86intrin.h>

#define VECTOR_WIDTH 8 // elements

U64 read_cpu_timer() { return __rdtsc(); }

// from
// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
U64 next_power_of_2(U64 n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

void prefix_sum_blelloch(S32 *input, S32 *output, S64 count) {
    S64 n = next_power_of_2(count);
    S32 *padded_input = calloc(n, sizeof(S32));
    memcpy(padded_input, input, count * sizeof(S32));

    // Up-sweep (reduction) phase
    for (S64 d = 0; d < log2(n); d++) {
        for (S64 k = 0; k < n; k += (1 << (d + 1))) {
            padded_input[k + (1 << (d + 1)) - 1] +=
                padded_input[k + (1 << d) - 1];
        }
    }

    // Down-sweep phase
    padded_input[n - 1] = 0;
    for (S64 d = log2(n) - 1; d >= 0; d--) {
        for (S64 k = 0; k < n; k += (1 << (d + 1))) {
            S32 t = padded_input[k + (1 << d) - 1];
            padded_input[k + (1 << d) - 1] =
                padded_input[k + (1 << (d + 1)) - 1];
            padded_input[k + (1 << (d + 1)) - 1] += t;
        }
    }

    memcpy(output, padded_input, count * sizeof(S32));
    free(padded_input);
}

typedef struct {
    S32 *data;
    S64 n;
    S64 d;
} UpSweepWork;

void up_sweep_task(void *arg) {
    UpSweepWork *work = (UpSweepWork *)arg;
    for (S64 k = 0; k < work->n; k += (1 << (work->d + 1))) {
        work->data[k + (1 << (work->d + 1)) - 1] +=
            work->data[k + (1 << work->d) - 1];
    }
}

typedef struct {
    S32 *data;
    S64 n;
    S64 d;
} DownSweepWork;

void down_sweep_task(void *arg) {
    DownSweepWork *work = (DownSweepWork *)arg;
    for (S64 k = 0; k < work->n; k += (1 << (work->d + 1))) {
        S32 t = work->data[k + (1 << work->d) - 1];
        work->data[k + (1 << work->d) - 1] =
            work->data[k + (1 << (work->d + 1)) - 1];
        work->data[k + (1 << (work->d + 1)) - 1] += t;
    }
}

void prefix_sum_blelloch_multithreaded(S32 *input, S32 *output, S64 count,
                                       int num_threads) {
    S64 n = next_power_of_2(count);
    S32 *padded_input = calloc(n, sizeof(S32));
    memcpy(padded_input, input, count * sizeof(S32));

    ThreadPool *pool = thread_pool_create(num_threads, log2(n) * 2);

    // Up-sweep (reduction) phase
    for (S64 d = 0; d < log2(n); d++) {
        UpSweepWork *work = malloc(sizeof(UpSweepWork));
        work->data = padded_input;
        work->n = n;
        work->d = d;
        thread_pool_submit(pool, up_sweep_task, work);
    }
    thread_pool_wait(pool);

    // Down-sweep phase
    padded_input[n - 1] = 0;
    for (S64 d = log2(n) - 1; d >= 0; d--) {
        DownSweepWork *work = malloc(sizeof(DownSweepWork));
        work->data = padded_input;
        work->n = n;
        work->d = d;
        thread_pool_submit(pool, down_sweep_task, work);
    }
    thread_pool_wait(pool);

    thread_pool_destroy(pool);

    memcpy(output, padded_input, count * sizeof(S32));
    free(padded_input);
}

void prefix_sum_vector(S32 *input, S32 *output) {
    __m256i v = _mm256_loadu_si256((__m256i *)input);
    __m256i zero = _mm256_setzero_si256();

    __m256i s =
        _mm256_alignr_epi8(v, _mm256_permute2x128_si256(zero, v, 0x20), 12);
    v = _mm256_add_epi32(v, s);

    s = _mm256_alignr_epi8(v, _mm256_permute2x128_si256(zero, v, 0x20), 8);
    v = _mm256_add_epi32(v, s);

    s = _mm256_alignr_epi8(v, _mm256_permute2x128_si256(zero, v, 0x20), 0);
    v = _mm256_add_epi32(v, s);

    v = _mm256_alignr_epi8(v, _mm256_permute2x128_si256(zero, v, 0x20), 12);

    _mm256_storeu_si256((__m256i *)output, v);
}

void prefix_sum_2pass(S32 *input, S32 *output, S64 count) {
    S64 num_chunks = count / VECTOR_WIDTH;

    // Pass 1: Intra-chunk prefix sums
    for (S64 i = 0; i < num_chunks; i++) {
        prefix_sum_vector(input + i * VECTOR_WIDTH, output + i * VECTOR_WIDTH);
    }

    // Extract chunk sums
    S32 *chunk_sums = malloc(sizeof(S32) * num_chunks);
    for (S64 i = 0; i < num_chunks; i++) {
        chunk_sums[i] = output[(i + 1) * VECTOR_WIDTH - 1] +
                        input[(i + 1) * VECTOR_WIDTH - 1];
    }

    // Prefix sum of chunk sums
    S32 *prefix_chunk_sums = malloc(sizeof(S32) * num_chunks);
    prefix_chunk_sums[0] = 0;
    for (S64 i = 1; i < num_chunks; i++) {
        prefix_chunk_sums[i] = prefix_chunk_sums[i - 1] + chunk_sums[i - 1];
    }

    // Pass 2: Final adjustment
    for (S64 i = 1; i < num_chunks; i++) {
        __m256i broadcast_sum = _mm256_set1_epi32(prefix_chunk_sums[i]);
        __m256i chunk_data =
            _mm256_loadu_si256((__m256i *)(output + i * VECTOR_WIDTH));
        chunk_data = _mm256_add_epi32(chunk_data, broadcast_sum);
        _mm256_storeu_si256((__m256i *)(output + i * VECTOR_WIDTH),
                              chunk_data);
    }

    free(chunk_sums);
    free(prefix_chunk_sums);
}
