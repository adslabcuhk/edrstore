/**
 * @file rabin_poly.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk) 
 * @brief a new rabin fp implementation (https://github.com/stevegt/librabinpoly)
 * @version 0.1
 * @date 2022-03-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_RABIN_PLOY_H 
#define MY_CODEBASE_RABIN_PLOY_H

#include "../define.h"

#define INT64(n) n##LL
#define MSB64 INT64(0x8000000000000000)

using namespace std;

typedef struct {
    uint32_t cur_pos;
    uint8_t* circ_buf;
    uint64_t fp;
} RabinCtx_t;

class RabinFPUtil {
    private:
        // configuration
        uint64_t fp_poly_;
        uint64_t window_size_;

        int shift_;
        uint64_t T_[256];			// Lookup table for mod
        uint64_t U_[256];

        uint64_t Append8(u_int64_t p, unsigned char m);

    public:
        /**
         * @brief Construct a new Rabin FP Util object
         * 
         * @param window_size the sliding window size
         */
        RabinFPUtil(uint64_t window_size);

        /**
         * @brief Destroy the Rabin FP Util object
         * 
         */
        ~RabinFPUtil();
        
        /**
         * @brief generate a new rabin fingerprint ctx
         * 
         * @param ctx the input ctx <ret>
         */
        void NewCtx(RabinCtx_t& ctx);

        /**
         * @brief reset the rabin fingerprint ctx
         * 
         * @param ctx the input ctx <ret>
         */
        void ResetCtx(RabinCtx_t& ctx);

        /**
         * @brief slide one byte in current ctx
         * 
         * @param ctx current ctx
         * @param input_byte the input byte
         * 
         * @return the new value of the Rabin fingerprint
         */
        uint64_t SlideOneByte(RabinCtx_t& ctx, uint8_t input_byte);

        /**
         * @brief free the rabin fingerprint ctx
         * 
         * @param ctx the input ctx
         */
        void FreeCtx(RabinCtx_t& ctx);
};

#endif