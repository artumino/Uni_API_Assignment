#include "local_random.h"


int local_rand(unsigned int seed, unsigned int max)  // RAND_MAX assumed to be 32767
{
    return (unsigned int)((seed * 1103515245 + 12345)/65536) % max;
}
