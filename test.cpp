#include <bits/stdc++.h>
// #include <time.h>
#include <unistd.h>
// #include <chrono>
#include <random>
#include <ratio>
#include "vacuum.h"

// generate n 64-bit random numbers
void random_gen(int n, std::vector<uint64_t>& store, std::mt19937& rd) {
    store.resize(n);
    for (int i = 0; i < n; i++)
        store[i] = (uint64_t(rd()) << 32) + rd();
}

// generate n 64-bit random numbers
void random_gen_1(int n, uint64_t** store, std::mt19937& rd) {
    *store = new uint64_t[n + 128];
    for (int i = 0; i < n; i++)
        (*store)[i] = (uint64_t(rd()) << 32) + rd();
}
void test_vf_no_padding() {

    /*
        We implemented VF_no_padding from scratch.
        It supports fingerprint length from 4 to 16 bits, but we recommend to use fingerprint longer than 8 bits.
        This version aims at flexibility, so it is slower than VF_with_padding.
    */

    std::cout << "Testing vacuum filter..." << std::endl;

    int n = 1 << 25; // number of inserted keys
    int q = 10000000; // number of queries

    std::cout << "Keys number = " << n << std::endl;
    std::cout << "Queries number = " << q << std::endl;

    std::mt19937 rd(12821);
    std::vector<uint64_t> insKey;
    std::vector<uint64_t> alienKey;
    random_gen(n, insKey, rd);
    random_gen(q, alienKey, rd);

    VacuumFilter<uint16_t> vf(n, 4, 400);

    for (int i = 0; i < n; i++){
        if (vf.insert(insKey[i]) == false)
            std::cout << "Insertion fails when inserting " << i << "th key: " << insKey[i] << std::endl;
    }

    std::cout << "Load factor = " << vf.get_load_factor() << std::endl;

    for (int i = 0; i < n; i++){
        if (vf.lookup(insKey[i]) == false)
            std::cout << "False negative happens at " << i << "th key: " << insKey[i] << std::endl;
    }
    
    int false_positive_cnt = 0;

    for (int i = 0; i < q; i++)
        if (vf.lookup(alienKey[i]) == true)
            false_positive_cnt++;

    std::cout << "False positive rate = " << double(false_positive_cnt) / q << std::endl;
    std::cout << "Bits per key = " << vf.get_bits_per_item() << std::endl;

    for (int i = 0; i < n; i++)
        if (vf.del(insKey[i]) == false)
            std::cout << "Deletion fails when inserting " << i << "th key: " << insKey[i] << std::endl;

    std::cout << std::endl;
}

int main() {
    test_vf_no_padding();
    return 0;
}
