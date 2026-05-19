// Matheo Kesar (mostly claude generated, I reviewed it)

#include <stdio.h>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include "../hash.h"
#include "../kmere.h"
#include "../vacuum.h"
using namespace std;

const int STEP_SIZE = 10000;
const int LOOKUP_SAMPLE = 50000;

void run_incremental(const string& data_type, int k, const vector<uint64_t>& kmers, const vector<uint64_t>& aliens, ofstream& csv) {

    VacuumFilter<uint16_t> vf(kmers.size(), 4, 400);

    int total = kmers.size();
    int inserted = 0;
    int failed = 0;

    int sample_size = min((int)aliens.size(), LOOKUP_SAMPLE);
    vector<uint64_t> alien_sample(aliens.begin(), aliens.begin() + sample_size);

    while (inserted < total) {
        //  how many to insert
        int batch_end = min(inserted + STEP_SIZE, total);
        int batch_size = batch_end - inserted;

        // inserting
        auto t1 = chrono::high_resolution_clock::now();
        for (int i = inserted; i < batch_end; i++) {
            if (!vf.insert(kmers[i])) {
                failed++;
            }
        }
        auto t2 = chrono::high_resolution_clock::now();

        inserted = batch_end;

        double insert_time = chrono::duration<double>(t2 - t1).count();
        double insert_throughput = batch_size / insert_time / 1e6;

        // lookup + false positive rate on alien sample
        int false_positives = 0;
        auto t3 = chrono::high_resolution_clock::now();
        for (auto& alien : alien_sample) {
            if (vf.lookup(alien)) {
                false_positives++;
            }
        }
        auto t4 = chrono::high_resolution_clock::now();

        double lookup_time = chrono::duration<double>(t4 - t3).count();
        double lookup_throughput = sample_size / lookup_time / 1e6;
        double fpr = (double)false_positives / sample_size;

        double load_factor = vf.get_load_factor();
        double bits_per_item = vf.get_bits_per_item();

        csv << data_type << ","
            << k << ","
            << inserted << ","
            << load_factor << ","
            << insert_throughput << ","
            << lookup_throughput << ","
            << fpr << ","
            << bits_per_item << ","
            << failed << "\n";
    }

    cout << "Done: " << data_type << " k=" << k
         << " | inserted=" << inserted
         << " failed=" << failed << endl;
}

int main(int argc, char* argv[]) {
    // if (argc < 2) {
    //     cout << "Wrong number of arguments." << endl;
    //     return 1;
    // }

    // string fna_path = argv[1];
    string fna_path = "../GCF_000005845.2_ASM584v2_genomic.fna";
    vector<int> k_values = {10, 20, 50, 100, 200};

    ofstream csv("incremental.csv");
    csv << "data_type,k,items_inserted,load_factor,"
        << "insert_throughput_mops,lookup_throughput_mops,"
        << "false_positive_rate,bits_per_item,failed_inserts\n";

    // E. coli
    cout << "=== E.coli incremental benchmark ===" << endl;
    for (int k : k_values) {
        cout << "k = " << k << endl;
        vector<uint64_t> kmers = extract_kmer(fna_path, k);
        vector<uint64_t> aliens = generate_random_kmer(kmers, LOOKUP_SAMPLE);
        cout << "Extracted " << kmers.size() << " k-mers" << endl;
        run_incremental("ecoli", k, kmers, aliens, csv);
    }

    // Artificial data
    cout << "=== Artificial data incremental benchmark ===" << endl;
    vector<uint64_t> art_data = artificial_data(1000000);
    vector<uint64_t> art_aliens = generate_random_kmer(art_data, LOOKUP_SAMPLE);
    run_incremental("artificial", 0, art_data, art_aliens, csv);

    csv.close();
    cout << "Saved to incremental.csv" << endl;
    return 0;
}