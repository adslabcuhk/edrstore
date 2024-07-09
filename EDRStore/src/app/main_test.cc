/**
 * @file main_test.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief a test main program
 * @version 0.1
 * @date 2022-05-31
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/define.h"
#include <random>

using namespace std;

string my_name = "Test";

int main (int argc, char* argv[]) {
    uint64_t padding_seed = 10000;
    default_random_engine eng{padding_seed};

    uniform_int_distribution<uint64_t> uniform_range(0, UINT64_MAX);
    for (size_t i = 0; i < 10; i++) {
        cout << uniform_range(eng) << endl;
    }
    
    return 0;
}

// int main(int argc, char* argv[]) {
//     srand(tool::GetStrongSeed());
//     TwoPhaseEnc* test_two_phase = new TwoPhaseEnc();
//     uint32_t chunk_size = (1 << 13);
//     const char opt_string[] = "i:";

//     if (argc != sizeof(opt_string)) {
//         tool::Logging(my_name.c_str(), "wrong argc number.\n");
//         exit(EXIT_FAILURE);
//     }

//     int option;
//     string input_path;
//     while ((option = getopt(argc, argv, opt_string)) != -1) {
//         switch (option) {
//             case 'i': {
//                 input_path.assign(optarg);
//                 break;
//             }
//             case '?': {
//                 tool::Logging(my_name.c_str(), "error optopt: %c\n", optopt);
//                 tool::Logging(my_name.c_str(), "error opterr: %d\n", opterr);
//                 exit(EXIT_FAILURE);
//             }
//         }
//     }

//     uint8_t enc_chunk[ENC_MAX_CHUNK_SIZE] = {0};
//     uint8_t input_chunk[ENC_MAX_CHUNK_SIZE] = {0};
//     uint8_t dec_chunk[ENC_MAX_CHUNK_SIZE] = {0};

//     ifstream input_file_hdl;
//     input_file_hdl.open(input_path, ios_base::in | ios_base::binary);
//     if (!input_file_hdl.is_open()) {
//         tool::Logging(my_name.c_str(), "cannot open %s\n", input_path.c_str());
//         exit(EXIT_FAILURE);
//     }

//     bool is_end = false;
//     bool correct_test = true;
//     int read_byte_num = 0;
//     struct timeval stime;
//     struct timeval etime;
//     double enc_time = 0;
//     double dec_time = 0;
//     uint64_t total_enc_size = 0;
//     uint32_t enc_chunk_size = 0;
//     uint32_t dec_chunk_size = 0;
//     uint8_t key[CHUNK_HASH_SIZE];
//     memset(key, 1, CHUNK_HASH_SIZE);
//     while (!is_end) {
//         input_file_hdl.read((char*)input_chunk, chunk_size);
//         read_byte_num = input_file_hdl.gcount();
//         is_end = input_file_hdl.eof();
//         if (read_byte_num == 0) {
//             break;
//         }

//         total_enc_size += read_byte_num;
//         gettimeofday(&stime, NULL);
//         enc_chunk_size = test_two_phase->TwoPhaseEncChunk(input_chunk, read_byte_num,
//             key, enc_chunk);
//         gettimeofday(&etime, NULL);
//         enc_time += tool::GetTimeDiff(stime, etime);

//         gettimeofday(&stime, NULL);
//         dec_chunk_size = test_two_phase->TwoPhaseDecChunk(enc_chunk, enc_chunk_size, key,
//             dec_chunk);
//         gettimeofday(&etime, NULL);
//         dec_time += tool::GetTimeDiff(stime, etime);

//         if (correct_test) {
//              // check the size first
//             tool::Logging(my_name.c_str(), "read byte size: %lu\n", read_byte_num);
//             if (dec_chunk_size != read_byte_num) {
//                 cout << "dec length: " << dec_chunk_size << endl;
//                 tool::Logging(my_name.c_str(), "dec length is not correct.\n");
//                 exit(EXIT_FAILURE);
//             } else {
//                 if (memcmp(input_chunk, dec_chunk, read_byte_num) != 0) {
//                     tool::Logging(my_name.c_str(), "the dec content is wrong.\n");
//                     exit(EXIT_FAILURE);
//                 }
//             }
//             correct_test = false;
//             tool::Logging(my_name.c_str(), "correctness test is passed.\n");
//         }

//         if (is_end) {
//             break;
//         }
//     }

//     fprintf(stdout, "--------Test Result--------\n");
//     fprintf(stdout, "total enc data size (MiB): %lf\n", total_enc_size / 1024.0 / 1024.0);
//     fprintf(stdout, "enc speed (MiB/s): %lf\n", static_cast<double>(total_enc_size) / 
//         1024.0 / 1024.0 / enc_time);
//     fprintf(stdout, "dec speed (MiB/s): %lf\n", static_cast<double>(total_enc_size) / 
//         1024.0 / 1024.0 / dec_time);

//     input_file_hdl.close();
//     delete test_two_phase;
//     return 0;
// }