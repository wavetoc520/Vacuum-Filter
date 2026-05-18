#pragma once
#include <stdio.h>

// copied from original
uint64_t MurmurHash64(uint64_t h)
{
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;   
    return h;
}
