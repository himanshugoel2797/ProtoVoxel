#include "rle.h"
#include <x86intrin.h>

uint32_t rle_encode(uint8_t *raw_Data, uint32_t len, uint8_t *dst)
{
    uint64_t *raw_Data_u64 = (uint64_t *)raw_Data;
    int32_t c_runLen = 0;
    uint32_t c_val = raw_Data[0];
    __m256i cur_val;
    __m256i next_val = _mm256_load_si256((__m256i *)&raw_Data_u64[0]);
    uint8_t *comp_ptr = (uint8_t *)dst;

    raw_Data += 32;
    for (int j = 1; j < len / (sizeof(uint64_t) * 4); j++)
    {
        cur_val = next_val;
        next_val = _mm256_load_si256((__m256i *)&raw_Data_u64[j * 4]);
        __m256i cur_val_shifted = _mm256_alignr_epi8(
            _mm256_permute2x128_si256(cur_val, cur_val, _MM_SHUFFLE(2, 0, 0, 1)),
            cur_val, 1); //_mm256_bsrli_epi128(cur_val, 1);
        cur_val_shifted = _mm256_insert_epi8(cur_val_shifted, raw_Data[0], 31);
        __m256i equality_mask_vec = _mm256_xor_si256(cur_val_shifted, cur_val);
        equality_mask_vec =
            _mm256_cmpeq_epi8(equality_mask_vec, _mm256_setzero_si256());
        uint32_t equality_mask = ~_mm256_movemask_epi8(equality_mask_vec);

        int32_t avail_bits = 32;
        for (; avail_bits > 0;)
        {
            uint32_t zero_bits = __tzcnt_u32(equality_mask);
            if (zero_bits > avail_bits)
                zero_bits = avail_bits;

            c_runLen += zero_bits;
            avail_bits -= zero_bits;
            equality_mask >>= zero_bits;

            if (avail_bits > 0)
            {
                // if (c_runLen > 0)
                {
                    do
                    {
                        uint8_t c_r = c_runLen >= 255;
                        comp_ptr[0] = c_r ? 255 : c_runLen;
                        c_runLen -= c_r ? 255 : c_runLen;
                        comp_ptr[1] = c_val;
                        comp_ptr += 2;
                    } while (c_runLen > 0);
                    c_val = raw_Data[1 - avail_bits];
                }
                avail_bits--;
                equality_mask >>= 1;
            }
        }
        raw_Data += 32;
    }
    if (c_runLen != 0)
    {
        comp_ptr[0] = c_runLen;
        comp_ptr[1] = c_val;
        comp_ptr += 2;
    }

    return (comp_ptr - dst);
}

uint32_t rle_decode(uint8_t *comp_data_space, uint32_t rle_len,
                    uint8_t *decomp_pool)
{
    uint8_t *tmp_ptr = (uint8_t *)decomp_pool;
    uint8_t *comp_ptr_8 = (uint8_t *)comp_data_space;
    uint32_t iter = 0;

    for (int j = 0; j < rle_len / 8; j++)
    {
        uint8_t *tmp_ptr_u8;
        int32_t len;
        uint8_t v;

        {
            len = comp_ptr_8[0];
            v = comp_ptr_8[(0 + 1)];
            tmp_ptr_u8 = (uint8_t *)&tmp_ptr[iter];
            iter += len + 1;
            len /= 16;
            __m128i c16 = _mm_set1_epi8(v);
            while (len-- >= 0)
            {
                _mm_storeu_si128((__m128i *)tmp_ptr_u8, c16);
                tmp_ptr_u8 += 16;
            }
        }
        {
            len = comp_ptr_8[2];
            v = comp_ptr_8[(2 + 1)];
            tmp_ptr_u8 = (uint8_t *)&tmp_ptr[iter];
            iter += len + 1;
            len /= 16;
            __m128i c16 = _mm_set1_epi8(v);
            while (len-- >= 0)
            {
                _mm_storeu_si128((__m128i *)tmp_ptr_u8, c16);
                tmp_ptr_u8 += 16;
            }
        }
        {
            len = comp_ptr_8[4];
            v = comp_ptr_8[(4 + 1)];
            tmp_ptr_u8 = (uint8_t *)&tmp_ptr[iter];
            iter += len + 1;
            len /= 16;
            __m128i c16 = _mm_set1_epi8(v);
            while (len-- >= 0)
            {
                _mm_storeu_si128((__m128i *)tmp_ptr_u8, c16);
                tmp_ptr_u8 += 16;
            }
        }
        {
            len = comp_ptr_8[6];
            v = comp_ptr_8[(6 + 1)];
            tmp_ptr_u8 = (uint8_t *)&tmp_ptr[iter];
            iter += len + 1;
            len /= 16;
            __m128i c16 = _mm_set1_epi8(v);
            while (len-- >= 0)
            {
                _mm_storeu_si128((__m128i *)tmp_ptr_u8, c16);
                tmp_ptr_u8 += 16;
            }
        }
        comp_ptr_8 += 8;
    }

    rle_len = rle_len & 0x7;
    if (rle_len > 0)
    {
        uint8_t *tmp_ptr_u8;
        int32_t len;
        uint8_t v;

        {
            len = comp_ptr_8[0];
            v = comp_ptr_8[(0 + 1)];
            tmp_ptr_u8 = (uint8_t *)&tmp_ptr[iter];
            iter += len + 1;
            len /= 16;
            __m128i c16 = _mm_set1_epi8(v);
            while (len-- >= 0)
            {
                _mm_storeu_si128((__m128i *)tmp_ptr_u8, c16);
                tmp_ptr_u8 += 16;
            }
        }

        if (rle_len > 2)
        {
            {
                len = comp_ptr_8[2];
                v = comp_ptr_8[(2 + 1)];
                tmp_ptr_u8 = (uint8_t *)&tmp_ptr[iter];
                iter += len + 1;
                len /= 16;
                __m128i c16 = _mm_set1_epi8(v);
                while (len-- >= 0)
                {
                    _mm_storeu_si128((__m128i *)tmp_ptr_u8, c16);
                    tmp_ptr_u8 += 16;
                }
            }
        }
        if (rle_len > 4)
        {
            {
                len = comp_ptr_8[4];
                v = comp_ptr_8[(4 + 1)];
                tmp_ptr_u8 = (uint8_t *)&tmp_ptr[iter];
                iter += len + 1;
                len /= 16;
                __m128i c16 = _mm_set1_epi8(v);
                while (len-- >= 0)
                {
                    _mm_storeu_si128((__m128i *)tmp_ptr_u8, c16);
                    tmp_ptr_u8 += 16;
                }
            }
        }
    }

    return iter;
}