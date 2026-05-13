// Nika Valić
// Reads a FNA file and extracts random k-mers encoded as uint64_t values
// Generates random k-mers that don't exist in the extracted k-mers for testing
// Generates random uint64_t values for testing purposes

#include <stdio.h>
#include <string>
#include <vector>
#include <random>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include "hash.h"
using namespace std;


// Hashes the k-mer string of length k from the starting position in the sequence
// For hashing we use the MurmurHash64 for encoding the k-mer as a 64-bit integer
uint64_t hash_kmer(const string& seq, int start, int k){
    uint64_t hash = 0;

    for(int i = 0; i < k; i++){
        hash = MurmurHash64(hash ^ (uint64_t)seq[start + i]); 
    }
    return hash;
}

// Reading the FNA file which consists of DNA sequences and extracted the k-mers from it,
// for different values of k and storing them in a vector 
// We skip the header and slide the winow of size k over the sequence
vector<uint64_t> extract_kmer(const string& file_path, int k){
    ifstream file(file_path);
    string seq = "";
    string line;

    if(!file.is_open()){
        cout << "Error opening file.";
        return {};
    }

    while(getline(file, line)){
        if(line.empty()){
            continue;
        }
        if(line[0] == '>'){
            continue; //  Skipping the header
        }
        seq += line;
    }
    file.close();

    vector<uint64_t> kmers;
    for(int i = 0; i <= (int)seq.size(); i++){
        uint64_t kmer_hash = hash_kmer(seq, i, k);
        if(kmer_hash != 0){
            kmers.push_back(kmer_hash);
        }
    }

    return kmers;
}

// Generating random uint64_t values that don't exist in the extracted k-mers,
// so that we can test the false positive rate of the vacuum filter
vector<uint64_t> generate_random_kmer(const vector<uint64_t>& existing_kmer, int num){
    unordered_set<uint64_t> existing_set(existing_kmer.begin(), existing_kmer.end()); // All values in our existing k-mers
    vector<uint64_t> random_kmers;
    int seed = random_device{}();
    mt19937_64 rnd(seed); // Pseudo-random generator of 64-bit numbers

    while((int)random_kmers.size() < num){
        uint64_t val = rnd();
        if(val == 0){
            continue; // 0s are reserved for empty slots
        }

        if(existing_set.find(val) == existing_set.end()){
            random_kmers.push_back(val); // We're only adding values that aren't the the existing k-mers
        }
    }
    return random_kmers;
}


// Generating n random uint64_t values 
// that we can use for testing instead of genome data
vector<uint64_t> artificial_data(int n){
    vector<uint64_t> data;
    int seed = random_device{}();
    mt19937_64 rnd(seed); 

    while((int)data.size() < n){
        uint64_t val = rnd();
        if (val != 0){
            data.push_back(val);
        }
    }
    return data;
}
