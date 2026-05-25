// Matheo Kesar
// Implementation of the vacuum filter

#include <stdio.h>
#include <vector>
#include <random>

#include "hash.h"

#define LOAD_FACTOR 0.95

bool LoadFactorTest(int items, float a, float r, int L, int slots){
    // aproximation of the load
    // items    - maximum num of items
    // a        - target load factor (around 96%)
    // r        - multiplier for a - for a more certain prediction (around 95%)
    // L        - alternate range - number of sequential buckets
    // slots    - number of slots per bucket

    // num of buckets (multiple of L)
    int m = ceil(items / a / slots / L) * L;
    // target number of entries
    int N = slots * m * r * a;
    // number of chunks
    int c = m / L;
    // min chunk capacity
    float P = 0.97 * slots * L;
    // max load estimator - "Balls into Bins" problem...
    float D = (double)N/c + 3.0/2*sqrt(2.0*N/c*log(c));

    return (D < P) ? true : false;
}

int RangeSelection(int items, float a, float r, int slots){
    int L = 1;
    while (LoadFactorTest(items, a, r, L, slots) == false) L *= 2;
    return L;
}


// T fingerprint type, ex. uint16_t
template<typename T>
class VacuumFilter
{
private:
    int max_item;
    int n; // num of buckets
    int m; // slots per bucket
    int MaxEvicts;
    std::vector<T> table; // entire table for entries (initially all at 0)
    std::vector<int> L; // alternate range lengths
    int filled_cells;

public:
    VacuumFilter(int max_item, int m, int MaxEvicts) : max_item(max_item), m(m), MaxEvicts(MaxEvicts) {

        // L0 = RangeSelection(n, a, 1);
        // L1 = RangeSelection(n, a, 0.75)
        // Li = RangeSelection(n, a, 1 - i/K)

        this->L.resize(4);
        for (int i = 0; i < 4; i++)
            this->L[i] = RangeSelection(max_item, LOAD_FACTOR, 1 - i/4.0, this->m);
        this->L[3] *= 2;



        // final bucket num - has to be a multiple of the largest
        // alternate range length and >= of the total capacity:
        //      a - max_item / 0.96 / 4
        //      b - L[0]
        //      -> ((a + b - 1) / b) * b
        this->n = (((int)(max_item / LOAD_FACTOR / this->m) + this->L[0] - 1) / this->L[0]) * this->L[0];
        this->table.resize(this->n * this->m);
        this->filled_cells = 0;
    };
    ~VacuumFilter() {};

    bool insert(uint64_t x);
    bool del(uint64_t x);
    bool lookup(uint64_t x);

    T fingerprint(uint64_t x);
    unsigned int pos_hash(uint64_t x);
    unsigned int alt(unsigned int b, T f);
    double get_load_factor();
    double get_bits_per_item();
};

template<typename T>
bool VacuumFilter<T>::insert(uint64_t x)
{
    /*
    f = fingerprint(x)
    b1 = hash(x)
    b2 = alt(b1, f)

    if b1 or b2 has empty slot:
        put f into it
        return true

    b = rand_choose(b1,b2)
    for (i=0, i<MaxEvicts, i++):
        for f' in b:
            if alt(b,f') has empty slot:
                put f into f' slot
                put f' into empty
                return true

        s = random slot from b
        switch f i f from s
        b = alt(b, f) //new f from above

    return false
    */

    T f = fingerprint(x);
    unsigned int b1 = pos_hash(x);
    unsigned int b2 = alt(b1, f);

    for (int i = 0; i < this->m; i++){
        if (table[b1*this->m + i] == 0){
            table[b1*this->m + i] = f;
            this->filled_cells++;
            return true;
        }
        if (table[b2*this->m + i] == 0){
            table[b2*this->m + i] = f;
            this->filled_cells++;
            return true;
        }
    }

    unsigned int b = (rand() & 1) == 0 ? b1 : b2;

    // if MaxEvicts is reached, revert all changes
    // pair stack {position, fingerprint}
    std::vector<std::pair<unsigned int, T>> promjene;

    for (int i = 0; i < this->MaxEvicts; i++){
        // until MaxEvicts is reached
        T temp_f;
        for (int j = 0; j < this->m; j++){
            // goes over fingerprints of the selected bucket
            temp_f = table[b*this->m + j]; // f' in pseudocode
            unsigned int temp_b = alt(b, temp_f);
            for (int k = 0; k < this->m; k++){
                // searches empty slot
                if (table[temp_b*this->m + k] == 0){
                    // found empty slot, puts f' inside and old value into f
                    table[temp_b*this->m + k] = temp_f;
                    table[b*this->m + j] = f;
                    this->filled_cells++;
                    return true;
                }
            }
        }

        unsigned int s = b*this->m + (rand() % this->m);
        temp_f = table[s];
        promjene.push_back({s, table[s]});
        table[s] = f;
        f = temp_f;
        b = alt(b, f);
    }

    // reverting changes because it failed to insert
    for (auto rit = promjene.rbegin(); rit != promjene.rend(); rit++){
        table[rit->first] = rit->second;
    }

    return false;

}

template <typename T>
bool VacuumFilter<T>::del(uint64_t x)
{
    /*
    f = fingerprint(x)
    b1 = hash(x)
    b2 = alt(b1, f)

    if f in b1 or b2:
        remove f from one of them
        return true
    return false
    */

    T f = fingerprint(x);
    unsigned int b1 = pos_hash(x);
    unsigned int b2 = alt(b1, f);

    for (int i = 0; i < this->m; i++){
        if (table[b1*this->m + i] == f){
            table[b1*this->m + i] = 0;
            this->filled_cells--;
            return true;
        }
        if (table[b2*this->m + i] == f){
            table[b2*this->m + i] = 0;
            this->filled_cells--;
            return true;
        }
    }
    return false;
}

template <typename T>
bool VacuumFilter<T>::lookup(uint64_t x)
{
    /*
    f = fingerprint(x)
    b1 = hash(x)
    b2 = alt(b1, f)

    if f in b1 or b2:
        return true
    return false
    */

    T f = fingerprint(x);
    unsigned int b1 = pos_hash(x);
    unsigned int b2 = alt(b1, f);

    for (int i = 0; i < this->m; i++){
        if (table[b1*this->m + i] == f){
            return true;
        }
        if (table[b2*this->m + i] == f){
            return true;
        }
    }
    return false;
}



template <typename T>
T VacuumFilter<T>::fingerprint(uint64_t x){
    // map from [0, 2^64-1] to [0, 2^T_len-2] to [1, 2^T_len-1] (0 represents "empty slot")
    return (MurmurHash64(x ^ 0x99D4A66AF0A2321ULL) % ((1ULL << (sizeof(T) * 8)) - 1)) + 1; // == hash % (2^T_len - 1) + 1;
}

template <typename T>
unsigned int VacuumFilter<T>::pos_hash(uint64_t x){
    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    // map from (uint32_t)MurmurHash64 [0, 2^32-1] (32 bit hash) to [0, n-1]
    return ((uint32_t)MurmurHash64(x ^ 0x66A234CUL) * (uint64_t)this->n) >> 32; // == (hash / 2^32) * n
}

template <typename T>
unsigned int VacuumFilter<T>::alt(unsigned int b, T f)
{
    // selects range based on fingerprint, makes fingerprint from fingerprint ???
    // and puts it somewhere into alternate range
    // xor with first bucket (reversable) -> alt(alt(b,f),f) == b

    T f_hash = fingerprint(f);
    int l = this->L[f % 4] - 1; // 2^k - 1 -> creates a bitmask like 00011111 = 31
    int delta = f_hash & l; // modifies lower bits that represent the position inside chunk
    int alt = b ^ delta; // always stays inside same chunk with a random offset (never bigger than n!!)


    // algoritha 4, doesnt use L
    // int delta = fingerprint(f) % this->n;
    // int alt = (b - delta) % this->n;
    // alt = (this->n - 1 - alt + delta) % this->n;

    return alt;
}

template <typename T>
double VacuumFilter<T>::get_load_factor(){
    // ratio of filled cells/slots and total capacity
    return this->filled_cells * 1.0 / (this->m * this->n);
}

template <typename T>
double VacuumFilter<T>::get_bits_per_item(){
    // ratio of total memory of dataset (+ vector overhead) and inserted data ->
    // average bits per fingerprint/item (big at first when not a lot of data is inserted)
    return (this->table.capacity() * sizeof(T) + sizeof(this->table)) * 8.0 / this->filled_cells;
}
