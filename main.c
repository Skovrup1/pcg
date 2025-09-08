#include "core.h"
#include "rand.h"

#include "rand.c"

int main() {
    printf("Hello sir!\n");

    printf("Your random number is %d\n", random_u32());

    return 0;
}
