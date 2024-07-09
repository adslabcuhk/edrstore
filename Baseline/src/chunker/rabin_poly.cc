/**
 * @file rabin_poly.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk) 
 * @brief a new rabin fp implementation (https://github.com/stevegt/librabinpoly)
 * @version 0.1
 * @date 2022-03-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/chunker/rabin_poly.h"

/*
     functions to calculate the rabin hash
*/

static inline uint32_t fls32(u_int32_t x) {
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

static inline char fls64 (uint64_t v) {
    uint32_t h;
    if ((h = v >> 32))
        return 32 + fls32 (h);
    else
        return fls32 ((uint32_t) v);
}

static u_int64_t polymod (u_int64_t nh, u_int64_t nl, u_int64_t d) {
    int i;
    int k = fls64 (d) - 1;
    d <<= 63 - k;

    if (nh) {
        if (nh & MSB64)
            nh ^= d;  // XXX unreachable? (on 32 bit platform?)
        for (i = 62; i >= 0; i--)
            if (nh & ((u_int64_t) 1) << i) {
                nh ^= d >> (63 - i);
                nl ^= d << (i + 1);
            }
    }
    for (i = 63; i >= k; i--)
    {
        if (nl & INT64 (1) << i)
            nl ^= d >> (63 - i);
    }
    return nl;
}

static void polymult (
        u_int64_t *php, u_int64_t *plp, u_int64_t x, u_int64_t y) {
    int i;
    u_int64_t ph = 0, pl = 0;
    if (x & 1)
        pl = y;
    for (i = 1; i < 64; i++)
        if (x & (INT64 (1) << i)) {
            ph ^= y >> (64 - i);
            pl ^= y << i;
        }
    if (php)
        *php = ph;
    if (plp)
        *plp = pl;
}

static u_int64_t polymmult (u_int64_t x, u_int64_t y, u_int64_t d) {
    u_int64_t h, l;
    polymult (&h, &l, x, y);
    return polymod (h, l, d);
}

/**
 * @brief Construct a new Rabin FP Util object
 * 
 * @param window_size the sliding window size
 */
RabinFPUtil::RabinFPUtil(uint64_t window_size) {
    fp_poly_ = 0xbfe6b8a5bf378d83LL;
    window_size_ = window_size;
    uint32_t i;
    int x_shift = fls64(fp_poly_) - 1;
    shift_ = x_shift - 8;

    uint64_t T1 = polymod (0, INT64 (1) << x_shift, fp_poly_);
    for (i = 0; i < 256; i++) {
        T_[i] = polymmult(i, T1, fp_poly_) | ((uint64_t) i << x_shift);
    }

    uint64_t sizeshift = 1;
    for (i = 1; i < window_size_; i++) {
        sizeshift = Append8(sizeshift, 0);
    }

    for (i = 0; i < 256; i++) {
        U_[i] = polymmult(i, sizeshift, fp_poly_);
    }
}

RabinFPUtil::~RabinFPUtil() {
    ;
}

uint64_t RabinFPUtil::Append8(uint64_t p, uint8_t m) {
    return ((p << 8) | m) ^ T_[p >> shift_];
}

/**
 * @brief generate a new rabin fingerprint ctx
 * 
 * @param ctx the input ctx <ret>
 */
void RabinFPUtil::NewCtx(RabinCtx_t& ctx) {
    ctx.circ_buf = (uint8_t*) malloc(window_size_ * sizeof(uint8_t));
    memset(ctx.circ_buf, 0, window_size_);
    ctx.cur_pos = -1;
    ctx.fp = 0;
    return ;
}

/**
 * @brief free the rabin fingerprint ctx
 * 
 * @param ctx the input ctx
 */
void RabinFPUtil::FreeCtx(RabinCtx_t& ctx) {
    free(ctx.circ_buf);
    return ;
}

/**
 * @brief reset the rabin fingerprint ctx
 * 
 * @param ctx the input ctx <ret>
 */
void RabinFPUtil::ResetCtx(RabinCtx_t& ctx) {
    memset(ctx.circ_buf, 0, window_size_);
    ctx.cur_pos = -1;
    ctx.fp = 0;
    return ;
}

/**
 * @brief slide one byte in current ctx
 * 
 * @param ctx current ctx
 * @param input_byte the input byte
 * 
 * @return the new value of the Rabin fingerprint
 */
uint64_t RabinFPUtil::SlideOneByte(RabinCtx_t& ctx, uint8_t input_byte) {
    ctx.cur_pos++;
    if (ctx.cur_pos >= window_size_) {
        ctx.cur_pos = 0;
    }
    uint8_t om = ctx.circ_buf[ctx.cur_pos];
    ctx.circ_buf[ctx.cur_pos] = input_byte;
    ctx.fp = Append8(ctx.fp ^ U_[om], input_byte);
    return ctx.fp; 
}