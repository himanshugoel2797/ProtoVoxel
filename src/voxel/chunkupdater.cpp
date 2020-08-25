#include "chunkupdater.h"
#include "rle.h"
#include <string.h>
#include <x86intrin.h>

namespace PVV = ProtoVoxel::Voxel;

PVV::ChunkUpdater::ChunkUpdater()
{
    active_chunk = nullptr;
}

PVV::ChunkUpdater::~ChunkUpdater()
{
}

void PVV::ChunkUpdater::UnpackChunk(PVV::Chunk *chnk)
{
    //Repack previous chunk if necessary
    if (active_chunk != nullptr && (active_state & ChunkState_UpdateDataPending))
    {
        //Repack
        uint32_t encoded_len = rle_encode(decompressed_cache, Chunk::ChunkLen, compressed_cache);
        delete[] active_chunk->compressed_data;
        active_chunk->compressed_data = new uint8_t[encoded_len];
        memcpy(active_chunk->compressed_data, compressed_cache, encoded_len);
        active_chunk->compressed_len = encoded_len;
        //TODO: Per thread memory pool, allows each thread to make an excessive allocation which can be shrunk back, allowing pack operations to function without a copy
    }

    active_chunk = chnk;
    if (active_chunk->compressed_len == 0)
        active_state = ChunkState_Uninitialized;
    else
        active_state = ChunkState_InitiallyPacked;
}

void PVV::ChunkUpdater::DecodeAll(bool mask_update_needed)
{
    if (active_state & ChunkState_Unpacked)
        return;
    if (active_state == ChunkState_Uninitialized)
    {
        memset(decompressed_cache, 0, Chunk::ChunkLen);
        memset(mask_cache, 0, Chunk::ChunkLen / sizeof(uint64_t));
        active_state = static_cast<ChunkState>(ChunkState_UnpackedData | ChunkState_UnpackedMasks);
        return;
    }

    if (!(active_state & ChunkState_UpdateDataPending))
    {
        rle_decode(active_chunk->compressed_data, active_chunk->compressed_len, decompressed_cache);
        active_state = static_cast<ChunkState>(active_state | ChunkState_UpdateMasksPending | ChunkState_UnpackedData);
    }
    if (mask_update_needed && (active_state & ChunkState_UpdateMasksPending))
    {
        uint8_t *vxl_u8_ = decompressed_cache;
        __m128i zr_vec = _mm_setzero_si128();
        uint64_t *vismask_l_p = mask_cache;
        for (uint32_t i = 0; i < 32 * 32; i++)
        {
            uint64_t m;
            __m128i vec_0 = _mm_load_si128((__m128i *)&vxl_u8_[0]);
            __m128i vec_1 = _mm_load_si128((__m128i *)&vxl_u8_[16]);
            __m128i vec_2 = _mm_load_si128((__m128i *)&vxl_u8_[32]);
            __m128i vec_3 = _mm_load_si128((__m128i *)&vxl_u8_[48]);

            uint64_t lbtm = _mm_movemask_epi8(_mm_cmpeq_epi8(vec_0, zr_vec));
            uint64_t ltop = _mm_movemask_epi8(_mm_cmpeq_epi8(vec_1, zr_vec));
            uint64_t hbtm = _mm_movemask_epi8(_mm_cmpeq_epi8(vec_2, zr_vec));
            uint64_t htop = _mm_movemask_epi8(_mm_cmpeq_epi8(vec_3, zr_vec));

            m = lbtm | (ltop << 16) | (hbtm << 32) | (htop << 48);
            vxl_u8_ += 64;
            *(vismask_l_p++) = ~m;
        }
        active_state = static_cast<ChunkState>((active_state & ~ChunkState_UpdateMasksPending) | ChunkState_UnpackedMasks);
    }
}

uint32_t PVV::ChunkUpdater::GetCompiledLength()
{
    //Only needs masks
    DecodeAll(true);

    auto visMask = mask_cache;
    auto cur_col_p = visMask + Chunk::ChunkSide;
    uint32_t expectedLen = 0;

    if (active_chunk->border_voxel_cnt == Chunk::BorderVoxelLen)
        return 0;
    if (active_chunk->set_voxel_cnt == Chunk::ChunkLen)
        return 0;
    if (active_chunk->set_voxel_cnt == 0)
        return 0;

    for (uint32_t x = 1 << 11; x < (Chunk::ChunkSide - 1) << 11; x += 1 << 11)
    {
        auto left_col = *cur_col_p++;
        auto cur_col_orig = *cur_col_p++;
        auto region = x / Chunk::RegionSize;

        if (active_chunk->regional_voxel_cnt[region] > 0)
            for (uint32_t z = 0; z < (Chunk::ChunkSide - 2); z++)
            {
                auto right_col = *cur_col_p++;

                if (cur_col_orig != 0)
                {
                    auto top_col = *(cur_col_p - Chunk::ChunkSide - 2); //top_col_p++;
                    auto btm_col = *(cur_col_p + Chunk::ChunkSide - 2); //btm_col_p++;

                    uint64_t cur_col = __andn_u64(cur_col_orig << 1, cur_col_orig) >> 1;
                    expectedLen += _mm_popcnt_u64(cur_col);

                    uint64_t cur_col2 = __andn_u64((int64_t)cur_col_orig >> 1, cur_col_orig) >> 1;
                    expectedLen += _mm_popcnt_u64(cur_col2);

                    uint64_t top_vis = __andn_u64(top_col, cur_col_orig) >> 1; //(top_col ^ cur_col) & cur_col;
                    expectedLen += _mm_popcnt_u64(top_vis);

                    uint64_t btm_vis = __andn_u64(btm_col, cur_col_orig) >> 1; //(btm_col ^ cur_col) & cur_col;
                    expectedLen += _mm_popcnt_u64(btm_vis);

                    uint64_t left_vis = __andn_u64(left_col, cur_col_orig) >> 1; //(left_col ^ cur_col) & cur_col;
                    expectedLen += _mm_popcnt_u64(left_vis);

                    uint64_t right_vis = __andn_u64(right_col, cur_col_orig) >> 1; //(right_col ^ cur_col) & cur_col;
                    expectedLen += _mm_popcnt_u64(right_vis);
                }

                left_col = cur_col_orig;
                cur_col_orig = right_col;
            }
        else
            cur_col_p += Chunk::ChunkSide - 2;
    }
    return expectedLen * 6;
}

const uint32_t backFace = 0 << 29;
const uint32_t frontFace = 1 << 29;
const uint32_t topFace = 2 << 29;
const uint32_t btmFace = 3 << 29;
const uint32_t leftFace = 4 << 29;
const uint32_t rightFace = 5 << 29;

const uint32_t y_off = 8;
const uint32_t x_off = 19;
const uint32_t z_off = 14;

static inline uint32_t *buildFace(uint32_t *inds, uint32_t xyz, uint8_t mat, uint32_t face)
{
    uint32_t x_axis;
    uint32_t y_axis;
    uint32_t cmn_axis;

    switch (face)
    {
    case leftFace:
        cmn_axis = (1 << z_off);
        x_axis = (1 << x_off); //x
        y_axis = (1 << y_off); //y
        break;
    case rightFace:
        cmn_axis = 0;
        x_axis = (1 << x_off); //x
        y_axis = (1 << y_off); //y
        break;
    case backFace:
        cmn_axis = (1 << y_off); //y
        x_axis = (1 << x_off);   //x
        y_axis = (1 << z_off);   //z
        break;
    case frontFace:
        cmn_axis = 0;
        x_axis = (1 << x_off); //x
        y_axis = (1 << z_off); //z
        break;
    case topFace:
        cmn_axis = (1 << x_off); //x
        x_axis = (1 << y_off);   //y
        y_axis = (1 << z_off);   //z
        break;
    case btmFace:
        cmn_axis = 0;
        x_axis = (1 << y_off); //y
        y_axis = (1 << z_off); //z
        break;
    }

    auto base_v = ((xyz << 8) | mat);
    auto base_v_cmn = base_v + cmn_axis;
    auto base_v_x = base_v_cmn + x_axis;
    auto base_v_y = base_v_cmn + y_axis;
    auto base_v_x_y = base_v_cmn + x_axis + y_axis;

    switch (face)
    {
    case backFace:
    case rightFace:
    case btmFace:
        inds[3] = inds[0] = base_v_cmn;
        inds[2] = base_v_x;
        inds[4] = base_v_y;
        inds[5] = inds[1] = base_v_x_y;
        break;
    case topFace:
    case leftFace:
    case frontFace:
        inds[3] = inds[0] = base_v_cmn;
        inds[1] = base_v_x;
        inds[5] = base_v_y;
        inds[4] = inds[2] = base_v_x_y;
        break;
    }
    return inds + 6;
}

static inline uint32_t *packFaces_c(uint32_t start_xyz, uint32_t end_xyz, uint8_t mat, uint32_t *inds, uint32_t face)
{
    uint32_t x_axis;
    uint32_t y_axis;
    uint32_t cmn_axis;

    switch (face)
    {
    case leftFace:
        cmn_axis = (1 << z_off);
        x_axis = (1 << x_off); //x
        y_axis = (1 << y_off); //y
        break;
    case rightFace:
        cmn_axis = 0;
        x_axis = (1 << x_off); //x
        y_axis = (1 << y_off); //y
        break;
    case topFace:
        cmn_axis = (1 << x_off); //x
        x_axis = (1 << y_off);   //y
        y_axis = (1 << z_off);   //z
        break;
    case btmFace:
        cmn_axis = 0;
        x_axis = (1 << y_off); //y
        y_axis = (1 << z_off); //z
        break;
    }

    auto base_v_end = ((end_xyz << 8) | mat) + cmn_axis;
    auto base_v = ((start_xyz << 8) | mat) + cmn_axis;

    switch (face)
    {
    case rightFace:
        inds[3] = inds[0] = base_v;
        inds[5] = inds[1] = base_v_end + x_axis + y_axis;
        inds[2] = base_v + x_axis;
        inds[4] = base_v_end + y_axis;
        break;
    case btmFace:
        inds[3] = inds[0] = base_v;
        inds[5] = inds[1] = base_v_end + x_axis + y_axis;
        inds[2] = base_v_end + x_axis;
        inds[4] = base_v + y_axis;
        break;
    case topFace:
        inds[3] = inds[0] = base_v;
        inds[2] = inds[4] = base_v_end + x_axis + y_axis;
        inds[1] = base_v_end + x_axis;
        inds[5] = base_v + y_axis;
        break;
    case leftFace:
        inds[3] = inds[0] = base_v;
        inds[4] = inds[2] = base_v_end + x_axis + y_axis;
        inds[1] = base_v + x_axis;
        inds[5] = base_v_end + y_axis;
        break;
    }
    return inds + 6;
}

uint32_t PVV::ChunkUpdater::Compile(uint32_t *inds_p)
{
    //Needs full data decompress
    DecodeAll(true);
    auto visMask = mask_cache;
    auto vxl_u8_sh = decompressed_cache + 1;
    auto cur_col_p = visMask + Chunk::ChunkSide;
    auto inds_orig = inds_p;

    if (active_chunk->border_voxel_cnt == Chunk::BorderVoxelLen) //Completely surrounded
        return 0;
    if (active_chunk->set_voxel_cnt == Chunk::ChunkLen) //Chunk is full and neighbors are full too, chunk is invisible
        return 0;
    if (active_chunk->set_voxel_cnt == 0)
        return 0;

    for (uint32_t x = 1 << 11; x < (Chunk::ChunkSide - 1) << 11; x += 1 << 11)
    {
        auto left_col = *cur_col_p++;
        auto cur_col_orig = *cur_col_p++;
        auto region = x / Chunk::RegionSize;

        if (active_chunk->regional_voxel_cnt[region] > 0)
            for (uint32_t z = 1 << 6; z < (Chunk::ChunkSide - 1) << 6; z += 1 << 6)
            {
                auto right_col = *cur_col_p++;

                if (cur_col_orig != 0)
                {
                    auto top_col = *(cur_col_p - Chunk::ChunkSide - 2); //top_col_p++;
                    auto btm_col = *(cur_col_p + Chunk::ChunkSide - 2); //btm_col_p++;
                    auto xz = z | x;

#define MESH_LOOP(col, face)                                                          \
    {                                                                                 \
        if (col != 0)                                                                 \
        {                                                                             \
            uint32_t start_fidx = xz | _tzcnt_u64(col);                               \
            uint32_t fidx = start_fidx;                                               \
            uint8_t cur_mat = vxl_u8_sh[fidx];                                        \
            uint32_t next_fidx;                                                       \
            uint8_t next_mat;                                                         \
            uint64_t next_col;                                                        \
            do                                                                        \
            {                                                                         \
                next_col = _blsr_u64(col);                                            \
                next_fidx = xz | _tzcnt_u64(next_col);                                \
                next_mat = vxl_u8_sh[next_fidx];                                      \
                uint64_t neighbor_test = (3ull << (fidx & 0x3f));                     \
                if ((cur_mat != next_mat) | ((col & neighbor_test) != neighbor_test)) \
                {                                                                     \
                    inds_p = packFaces_c(start_fidx, fidx, cur_mat, inds_p, face);    \
                    start_fidx = next_fidx;                                           \
                }                                                                     \
                col = next_col;                                                       \
                fidx = next_fidx;                                                     \
                cur_mat = next_mat;                                                   \
            } while (col != 0);                                                       \
        }                                                                             \
    }

#define MESH_DIRECT_LOOP(col, face)                              \
    while (col != 0)                                             \
    {                                                            \
        uint32_t fidx = xz | _tzcnt_u64(col);                    \
        col = _blsr_u64(col);                                    \
        inds_p = buildFace(inds_p, fidx, vxl_u8_sh[fidx], face); \
    }

                    uint64_t cur_col = __andn_u64(cur_col_orig << 1, cur_col_orig) >> 1;
                    MESH_DIRECT_LOOP(cur_col, frontFace);

                    uint64_t cur_col2 = __andn_u64((int64_t)cur_col_orig >> 1, cur_col_orig) >> 1;
                    MESH_DIRECT_LOOP(cur_col2, backFace);

                    uint64_t top_vis = __andn_u64(top_col, cur_col_orig) >> 1; //(top_col ^ cur_col) & cur_col;
                    MESH_LOOP(top_vis, btmFace);

                    uint64_t btm_vis = __andn_u64(btm_col, cur_col_orig) >> 1; //(btm_col ^ cur_col) & cur_col;
                    MESH_LOOP(btm_vis, topFace);

                    uint64_t left_vis = __andn_u64(left_col, cur_col_orig) >> 1; //(left_col ^ cur_col) & cur_col;
                    MESH_LOOP(left_vis, rightFace);

                    uint64_t right_vis = __andn_u64(right_col, cur_col_orig) >> 1; //(right_col ^ cur_col) & cur_col;
                    MESH_LOOP(right_vis, leftFace);
                }

                left_col = cur_col_orig;
                cur_col_orig = right_col;
            }
        else
            cur_col_p += Chunk::ChunkSide - 2;
    }

    return inds_p - inds_orig;
}

void PVV::ChunkUpdater::SetBlock(uint8_t x, uint8_t y, uint8_t z, uint8_t val)
{
    //Needs full data decompress
    DecodeAll(false);

    //TODO: Only queue a rebuild if the current block has changed
    uint32_t idx = Chunk::Encode(x, y, z);
    auto region = idx / Chunk::RegionSize;

    bool add_voxel = (decompressed_cache[idx] == 0 && val != -1);
    bool rm_voxel = (decompressed_cache[idx] != 0 && val == -1);
    if (add_voxel)
    {
        active_chunk->regional_voxel_cnt[region]++;
        active_chunk->set_voxel_cnt++;

        if ((x == 0) | (y == 0) | (z == 0) | (x == Chunk::ChunkSide - 1) | (y == Chunk::ChunkSide - 1) | (z == Chunk::ChunkSide - 1))
            active_chunk->border_voxel_cnt++;
    }
    decompressed_cache[idx] = val;
    if (rm_voxel)
    {
        active_chunk->regional_voxel_cnt[region]--;
        active_chunk->set_voxel_cnt--;

        if ((x == 0) | (y == 0) | (z == 0) | (x == Chunk::ChunkSide - 1) | (y == Chunk::ChunkSide - 1) | (z == Chunk::ChunkSide - 1))
            active_chunk->border_voxel_cnt--;
    }

    active_state = ChunkState_UpdatePending;
}

void PVV::ChunkUpdater::SetColumn(uint8_t x, uint8_t z, uint8_t val)
{
    //Needs full data decompress
    DecodeAll(false);
    //TODO: Implement column setting
    active_state = ChunkState_UpdatePending;
}

void PVV::ChunkUpdater::SetAll(uint8_t val)
{
    //Needs full data decompress
    DecodeAll(false);
    for (uint32_t x = 1 << 11; x < (Chunk::ChunkSide - 1) << 11; x += 1 << 11)
        for (uint32_t z = 1 << 6; z < (Chunk::ChunkSide - 1) << 6; z += 1 << 6)
        {
            auto xz = z | x;
            uint8_t *tmp_ptr = &decompressed_cache[xz | 1];
            for (int j = 1; j < 8; j++)
                *(tmp_ptr++) = val;

            uint64_t val_64 = 0x0101010101010101 * val;
            uint64_t *tmp_ptr_64 = (uint64_t *)tmp_ptr;
            for (int j = 0; j < 6; j++)
                *(tmp_ptr_64++) = val_64;

            tmp_ptr = (uint8_t *)tmp_ptr_64;
            for (int j = 1; j < 8; j++)
                *(tmp_ptr++) = val;
        }

    //TODO: specialized extra cheap pack
    //TODO: recompute chunk statistics
    active_state = ChunkState_UpdatePending;
}

void PVV::ChunkUpdater::RayCast()
{
    //Only needs masks, material data for intersection can be directly retrieved without decompression
    DecodeAll(true);
    //TODO: Implement chunk ray casting
}