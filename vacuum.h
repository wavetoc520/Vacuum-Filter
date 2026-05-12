// VacuumFilter<uint16_t> vf;

#include <stdio.h>
#include <vector>
#include <random>

#include "hash.h"

// vjerojatno ce bit pametno imat klasu koja ce se bavit sa semi sortiranjem - ovdje implementirat funkcije za dohvaćanje, izmjenu itd... (set_bucket, insert_to_bucket, lookup_in_bucket, ...)
// class Bucket
// {
// private:
    
// public:
//     Bucket(/* args */);
//     ~Bucket();
// };

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

public:
    VacuumFilter(int max_item, int m, int MaxEvicts) : max_item(max_item), m(m), MaxEvicts(MaxEvicts), n(max_item / m), table(max_item) { /* trebat ce dodat ono semi-sorting buckets*/};
    ~VacuumFilter() {};

    // ne znam koji tip da stavim (ovako je u official)
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
        for (int j = 0; j < this->m; j++){
            // prolazi fingerprinte odabranog bucketa
            T temp_f = table[b*this->m + j]; // f' u pseudokodu
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
        T temp_f = table[s];
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
    return false;
}



template <typename T>
T VacuumFilter<T>::fingerprint(__uint64_t x)
{
    // todooo
    // osiguraj da ne vrati 0 jer to koristim kao prazan slot
    return T();
}

template <typename T>
unsigned int VacuumFilter<T>::pos_hash(__uint64_t x){
    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    // ovo mapira iz (__uint32_t)MurmurHash64 [0, 2^32-1) (32 bitni hash) u [0, n-1)
    return ((__uint32_t)MurmurHash64(x) * this -> n) >> 32 // mogu li koristit MurmurHash32?
}

template <typename T>
unsigned int VacuumFilter<T>::alt(unsigned int b, T f)
{
    // todooooo
    return 0;
}
