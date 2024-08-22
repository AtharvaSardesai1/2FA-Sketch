#include "genzipf.h"
#include "murmur3.h"

#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <chrono>

#include <cstring> // for memcpy



using namespace std;


void generate_13_byte_key(const void* data, int len, uint32_t seed, uint8_t* out)
{
    uint32_t hash1 = MurmurHash3_x86_32(data, len, seed);
    uint32_t hash2 = MurmurHash3_x86_32(data, len, seed + 1);
    uint32_t hash3 = MurmurHash3_x86_32(data, len, seed + 2);
    
    memcpy(out, &hash1, 4);
    memcpy(out + 4, &hash2, 4);
    memcpy(out + 8, &hash3, 4);
    // Only take the first byte of the fourth hash result to make up 13 bytes
    uint32_t hash4 = MurmurHash3_x86_32(data, len, seed + 3);
    memcpy(out + 12, &hash4, 1);
}

uint32_t key_len = 13;
uint32_t flow_num = 0.10E6;
uint32_t packet_num = 3E6;
unordered_map<uint32_t, uint32_t> ground_truth;
map<uint32_t, uint32_t> fsd;



void gen_zipf_dataset(double alpha, int k)
{
    ground_truth.clear();
    fsd.clear();
    uint32_t hash_seed = std::chrono::steady_clock::now().time_since_epoch().count();

    // string filename = "zipf_" + to_string(alpha).substr(0, 3) + ".dat";
    string filename = to_string( k) + ".dat";
    cout << filename << endl;
    ofstream outFile(filename.c_str(), ios::binary);

    uint8_t key[13];
    for (int i = 0; i < packet_num; ++i)
    {
        uint32_t rand_num = zipf(alpha, flow_num, i == 0);
        // uint32_t key = MurmurHash3_x86_32(&rand_num, key_len, hash_seed);
        generate_13_byte_key(&rand_num, sizeof(rand_num), hash_seed, key);
        outFile.write((char *)&key, key_len);

        uint32_t key_hash = MurmurHash3_x86_32(key, 13, hash_seed);
        ground_truth[key_hash]++;
    }
    outFile.close();

    string statname = to_string( k) + ".stat";
    // string statname = "zipf_" + to_string(alpha).substr(0, 3) + ".stat";
    ofstream outStat(statname.c_str());

    cout << ground_truth.size() << " flows, " << packet_num << " packets" << endl;
    outStat << ground_truth.size() << " flows, " << packet_num << " packets" << endl;
    for (auto pr : ground_truth)
        fsd[pr.second]++;
    for (auto pr : fsd)
        outStat << pr.first << "\t\t" << pr.second << endl;
}

int main()
{
    for(int k = 0; k <=9; ++k){
    for (double alpha = 0.4; alpha < 0.41; alpha += 0.2)
    {
        cout << "generating zipf " << alpha << endl;
        gen_zipf_dataset(alpha, k);
    }
    }
    return 0;
}
