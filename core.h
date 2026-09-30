#ifndef CORE_H
#define CORE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int64_t S64;
typedef int32_t S32;
typedef int16_t S16;
typedef int8_t S8;

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

typedef float F32;
typedef double F64;

typedef bool Bool;

#define TARGET(arch) __attribute__((target(arch)))

#define NO_INLINE __attribute__((noinline))

#define COUNT(data) sizeof(data) / sizeof(data[0])

#define ENSURE(condition)                                                      \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "Ensure failed: %s, file %s, line %d\n",           \
                    #condition, __FILE__, __LINE__);                           \
            abort();                                                           \
        }                                                                      \
    } while (0)

#ifndef NDEBUG
#define ASSERT(condition)                                                      \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "Assert failed: %s, file %s, line %d\n",           \
                    #condition, __FILE__, __LINE__);                           \
            abort();                                                           \
        }                                                                      \
    } while (0)
#else
#define assert(condition) ((void)0)
#endif

#define ASSUME(cond)                                                           \
    do {                                                                       \
        if (!(cond))                                                           \
            __builtin_unreachable();                                           \
    } while (0)

#define KiB(count) count * 1024
#define MiB(count) count * 1024 * 1024
#define GiB(count) count * 1024 * 1024 * 1024
#define TiB(count) count * 1024 * 1024 * 1024 * 1024

#define PI 3.14159265358979323846

#define MAX_U32 4294967296

U64 read_cpu_timer();
void prefix_sum_blelloch(S32 *input, S32 *output, S64 count);
void prefix_sum_blelloch_multithreaded(S32 *input, S32 *output, S64 count,
                                       int num_threads);
void prefix_sum_vector(S32 *input, S32 *output);
void prefix_sum_2pass(S32 *input, S32 *output, S64 count);

#endif // CORE_H
