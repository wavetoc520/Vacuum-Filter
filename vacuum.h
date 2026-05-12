// VacuumFilter<uint16_t> vf;

#include <stdio.h>
#include <vector>
#include <random>

#include "hash.h"

#define LOAD_FACTOR 0.95

// vjerojatno ce bit pametno imat klasu koja ce se bavit sa semi sortiranjem - ovdje implementirat funkcije za dohvaćanje, izmjenu itd... (set_bucket, insert_to_bucket, lookup_in_bucket, ...)
// class Bucket
// {
// private:
    
// public:
//     Bucket(/* args */);
//     ~Bucket();
// };


bool LoadFactorTest(int items, float a, float r, int L, int slots){
    // ovo je aproksimacija na temelju neke matematike...idk
    // items    - ukupan broj itema
    // a        - ciljani load factor (valjda negdje 96%)
    // r        - multiplikator za a - da budemo sigurniji u rješenje (valjda negdje 95%)
    // L        - alternate range - broj uzastopnih bucketa
    // slots    - broj slotova po bucketu

    // broj bucketa ( mora biti višekratnik od L)
    int m = ceil(items / a / slots / L) * L;
    // ciljni broj itema
    int N = slots * m * r * a;
    // broj chunkova
    int c = m / L;
    // minimalni kapacitet svakog chunka
    float P = 0.97 * slots * L;
    // procjenitelj maksimalnog loada - "Balls into Bins" problem...
    float D = (double)N/c + 3.0/2*sqrt(2.0*N/c*log(c));

    return (D < P) ? true : false;
}

int RangeSelection(int items, float a, float r, int slots){
    int L = 1;
    while (LoadFactorTest(items, a, r, L, slots) == false) L *= 2;
    return L;
}


// T je tip fingerprinta, kao recimo __uint16_t
template<typename T>
class VacuumFilter
{
private:
    int max_item;
    int n; // num of buckets
    int m; // slots per bucket
    int MaxEvicts;
    std::vector<T> table; // cijelo polje (inicijalno na 0)
    std::vector<int> L; // alternate range duljine

public:
    VacuumFilter(int max_item, int m, int MaxEvicts) : max_item(max_item), m(m), MaxEvicts(MaxEvicts) {

        // L0 = RangeSelection(n, a, 1);
        // L1 = RangeSelection(n, a, 0.75)
        // Li = RangeSelection(n, a, 1 - i/K)

        this->L.resize(4);
        for (int i = 0; i < 4; i++)
            this->L[i] = RangeSelection(max_item, LOAD_FACTOR, 1 - i/4.0, this->m);
        this->L[3] *= 2;



        // konačni broj bucketa - želimo da bude višekratnik od najveće
        // alternate range duljine i veći ili jednak najvećem kapacitetu:
        //      a - max_item / 0.96 / 4
        //      b - L[0]
        //      -> ((a + b - 1) / b) * b
        this->n = (((int)(max_item / LOAD_FACTOR / this->m) + this->L[0] - 1) / this->L[0]) * this->L[0];
        this->table.resize(this->n * this->m);
        /* trebat ce dodat ono semi-sorting buckets*/
    };
    ~VacuumFilter() {};

    bool insert(__uint64_t x);
    bool del(__uint64_t x);
    bool lookup(__uint64_t x);

    T fingerprint(__uint64_t x);
    unsigned int pos_hash(__uint64_t x);
    unsigned int alt(unsigned int b, T f);
};


template<typename T>
bool VacuumFilter<T>::insert(__uint64_t x)
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
        b = alt(b, f) //f je sada taj zamijenjeni fingerprint iz f

    return false
    */

    T f = fingerprint(x);
    unsigned int b1 = pos_hash(x);
    unsigned int b2 = alt(b1, f);

    for (int i = 0; i < this->m; i++){
        if (table[b1*this->m + i] == 0){
            table[b1*this->m + i] = f;
            return true;
        }
        if (table[b2*this->m + i] == 0){
            table[b2*this->m + i] = f;
            return true;
        }
    }

    unsigned int b = (rand() & 1) == 0 ? b1 : b2;

    for (int i = 0; i < this->MaxEvicts; i++){
        // prolazi dok ne prijede MaxEvicts
        T temp_f;
        for (int j = 0; j < this->m; j++){
            // prolazi fingerprinte odabranog bucketa
            temp_f = table[b*this->m + j]; // f' u pseudokodu
            unsigned int temp_b = alt(b, temp_f);
            for (int k = 0; k < this->m; k++){
                // trazi empty slot
                if (table[temp_b*this->m + k] == 0){
                    // nasao empty slot pa stavlja f' u njega, a f umjesto njega
                    table[temp_b*this->m + k] = temp_f;
                    table[b*this->m + j] = f;
                    return true;
                }
            }
        }

        unsigned int s = b*this->m + (rand() % this->m);
        temp_f = table[s];
        table[s] = f;
        f = temp_f;
        b = alt(b, f);
    }

    return false;

}

template <typename T>
bool VacuumFilter<T>::del(__uint64_t x)
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
            return true;
        }
        if (table[b2*this->m + i] == f){
            table[b2*this->m + i] = 0;
            return true;
        }
    }
    return false;
}

template <typename T>
bool VacuumFilter<T>::lookup(__uint64_t x)
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
T VacuumFilter<T>::fingerprint(__uint64_t x){
    // mapira iz [0, 2^64-1] na [0, 2^T_len-2] na [1, 2^T_len-1] (0 ostavljamo za "prazan slot")
    return (MurmurHash64(x ^ 0x99D4A66AF0A2321ULL) % ((1ULL << (sizeof(T) * 8)) - 1)) + 1; // == hash % (2^T_len - 1) + 1;
}

template <typename T>
unsigned int VacuumFilter<T>::pos_hash(__uint64_t x){
    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    // ovo mapira iz (__uint32_t)MurmurHash64 [0, 2^32-1] (32 bitni hash) u [0, n-1]
    return ((__uint32_t)MurmurHash64(x ^ 0x66A234CUL) * this->n) >> 32; // == (hash / 2^32) * n
}

template <typename T>
unsigned int VacuumFilter<T>::alt(unsigned int b, T f)
{
    // na temelju fingerprinta odabire koji range će koristiti
    // radi novi fingerprint od fingerprinta ?? i stavlja ga u alternate range
    // xor sa prvim bucketor (invertibilno) -> alt(alt(b,f),f) == b

    T f_hash = fingerprint(f);
    int l = this->L[f % 4] - 1; // 2^k - 1 - radi bitmasku kao 00011111 = 31
    int delta = f_hash & l; // uzima samo donje bitove koji predstavljaju položaj unutar chunka
    int alt = b ^ delta; // uvijek ostaje unutar istog chunka s nekim random pomakom (nikad neće bit veće od n!!)


    // algoritam 4, ne koristi uopće L
    // int delta = fingerprint(f) % this->n;
    // int alt = (b - delta) % this->n;
    // alt = (this->n - 1 - alt + delta) % this->n;

    return alt;
}
