// Nika Valić
// Main function to test k-mer extraction and vacuum filter implementation
// Testing is done with E.coli genome sequence and artificial data
// Measuring false positive rate and load factor 

#include <stdio.h>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>


#include "hash.h"
#include "kmere.h"
#include "vacuum.h"

using namespace std;

void run_testing(const string& data_type, int k, const vector<uint64_t> kmers, ofstream& csv){
    vector<uint64_t> aliens = generate_random_kmer(kmers, 1000000);
    
    // Creatin the vacuum filter with the size of extracted k-mer,
    // 4 slots in each bucket and 400 max evictions
    VacuumFilter<uint16_t> vf(kmers.size(), 4, 400); // Taken from original implementation

    // Measuring insertion throughput
    auto time1 = chrono::high_resolution_clock::now();
    int failed_insert_cnt = 0;
    for(auto kmer : kmers){
        if(!vf.insert(kmer)){
            failed_insert_cnt++;
        }
    }
    auto time2 = chrono::high_resolution_clock::now();


    // Measuring lookup throughput and FPR
    auto time3 = chrono::high_resolution_clock::now();
    int false_positive_cnt = 0;
    for(auto alien : aliens){
        if(vf.lookup(alien)){
            false_positive_cnt++;
        }
    }
    auto time4 = chrono::high_resolution_clock::now();

    // Measuring deletion throughput
    auto time5 = chrono::high_resolution_clock::now();
    for(auto kmer : kmers){
        vf.del(kmer);
    }
    auto time6 = chrono::high_resolution_clock::now();

    // Calculating the metrics
    double insert_time = chrono::duration<double>(time2 - time1).count();
    double lookup_time = chrono::duration<double>(time4 - time3).count();
    double delete_time = chrono::duration<double>(time6 - time5).count();
    double insert_throughput = kmers.size() /insert_time / 1e6; // Million operations per second
    double lookup_throughput = aliens.size() /lookup_time / 1e6; // Million operations per second
    double false_positive_rate = double(false_positive_cnt) / aliens.size();
    double load_factor = vf.get_load_factor();
    double bits_per_item = vf.get_bits_per_item();

    cout << "Data: " << data_type << ", k: " << k << endl;
    cout << "Inserted: " << kmers.size() << " k-mers, failed: " << failed_insert_cnt << endl;
    cout << "False positive rate: " << false_positive_rate << endl;
    cout << "Insert throughput: " << insert_throughput << " MOPS" << endl;
    cout << "Lookup throughput: " << lookup_throughput << " MOPS" << endl;
    cout << "Delete throughput: " << kmers.size() / delete_time / 1e6 << " MOPS" << endl;
    cout << "Load factor: " << load_factor << endl;
    cout << "Bits per item: " << bits_per_item << endl;
    cout << endl;
    
    csv << data_type << ", " << k << ", " << false_positive_rate << ", " << insert_throughput << ", " << lookup_throughput << ", " << load_factor << ", " << bits_per_item << "\n";
}

int main(int argc, char* argv[]){
    if(argc < 2){
        cout << "Wrong number of arguments." << endl;
        return 1;
    }

    string fna_path  = argv[1];
    vector<int> k_values = {10, 20, 50, 100, 200};

    // Opening csv file for results
    ofstream csv("results/results.csv");
    csv << "data_type,k,false_positive_rate,insert_throughput_mops,lookup_throughput_mops,load_factor,bits_per_item\n";

    // E. coli genome experiment
    cout << "===E.coli genome testing====" << endl;
    for(int k: k_values){
        cout << "Testing for k = " << k << endl;
        vector<uint64_t>kmers = extract_kmer(fna_path, k);
        cout << "Number of k-mers: " << kmers.size() << endl;
        run_testing("ecoli", k, kmers, csv);
    }

    // Artificial data experiment
    cout << "===Artificial data testing===" << endl;
    vector<uint64_t> data = artificial_data(1000000);
    run_testing("artificial", 0, data, csv);

    csv.close();
    cout << "Results saved to results/results.csv" << endl;
    return 0;
}