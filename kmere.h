// Nika Valić
// Reads a FNA file and extracts random k-mers encoded as uint64_t values

#include <stdio.h>
#include <vector>
#include <random>
#include <fstream>
#include <unordered_set>

#include "hash.h"
using namespace std;


// Hashes the k-mer string of length k from the starting position in the sequence
// For hashing we use the MurmurHash64 for encoding the k-mer as a 64-bit integer
uint64_t hash_kmer(const string& seq, int start, int k){}

// Reading the FNA file which consists of DNA sequences and extracted the k-mers from it,
// for different values of k and storing them in a vector 
// We skip the header and slide the winow of size k over the sequence
vector<uint64_t> extract_kmer(const string& file_path, int k){}

// Generating random uint64_t values that don't exist in the extracted k-mers,
// so that we can test the false positive rate of the vacuum filter
vector<uint64_t> generate_random_kmer(const vector<uint64_t>& existing_kmer, int num){}


// Generating n random uint64_t values 
// that we can use for testing instead of genome data
vector<uint64_t> artificial_data(int n){}
