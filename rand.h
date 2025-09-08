#ifndef RAND_H
#define RAND_H

#include "core.h"

// [0, MAX_U32)
U32 random_u32();

// [0, bound)
U32 random_bound_u32(U32 bound);

// [min, max]
U32 random_interval_u32(U32 min, U32 max);

#endif // RAND_H
