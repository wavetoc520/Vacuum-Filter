#include <stdio.h>

// kopirao iz originala - pogledaj od kud je to doslo
__uint64_t MurmurHash64(__uint64_t h)
{
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;   
    return h;
}
