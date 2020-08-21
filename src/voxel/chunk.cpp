#include "chunk.h"
#include "mortoncode.h"
#include <string.h>
#include <x86intrin.h>

namespace PVV = ProtoVoxel::Voxel;

PVV::Chunk::Chunk()
{
}

void PVV::Chunk::Initialize(ChunkMalloc *mem_parent)
{
    this->mem_parent = mem_parent;
    vxl_u8 = nullptr;
    vismasks = nullptr;
    codingScheme = ChunkCodingScheme::SingleFull;
    allVal = -1;
    set_voxel_cnt = 0;
    border_voxel_cnt = 0;
}

void PVV::Chunk::SetAll(uint16_t val)
{
    codingScheme = ChunkCodingScheme::SingleFull;
    allVal = val;
    if (allVal != -1)
        set_voxel_cnt = ChunkLen;
    else
        set_voxel_cnt = 0;

    if (vxl_u8 != nullptr)
    {
        delete[] vxl_u8;
        vxl_u8 = nullptr;

        delete[] vismasks;
        vismasks = nullptr;
    }
}

void PVV::Chunk::SetSingle(uint8_t x, uint8_t y, uint8_t z, int16_t val)
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        uint32_t b_idx = EncodeXZ(x, z);
        auto vmask = vismasks[b_idx];
        auto mask = (1ull << y);
        uint32_t idx = EncodeXZ_Y(b_idx, y);
        auto region = idx / RegionSize;

        bool add_voxel = ((vmask & mask) == 0 && val != -1);
        bool rm_voxel = ((vmask & mask) != 0 && val == -1);
        vismasks[b_idx] = (vmask & ~mask) | (-(val != -1) & mask); //Set the visibility bit appropriately
        if (add_voxel)
        {
            if (regional_voxel_cnt[region] == 0)
                mem_parent->CommitChunkRegion(vxl_u8, region);
            regional_voxel_cnt[region]++;
            set_voxel_cnt++;

            if (x == 0 | y == 0 | z == 0 | x == ChunkSide - 1 | y == ChunkSide - 1 | z == ChunkSide - 1)
                border_voxel_cnt++;
        }
        vxl_u8[idx] = val;
        if (rm_voxel)
        {
            regional_voxel_cnt[region]--;
            if (regional_voxel_cnt[region] == 0)
                mem_parent->DecommitChunkBlock(vxl_u8, region);
            set_voxel_cnt--;

            if (x == 0 | y == 0 | z == 0 | x == ChunkSide - 1 | y == ChunkSide - 1 | z == ChunkSide - 1)
                border_voxel_cnt--;
        }
        break;
    }
    case ChunkCodingScheme::SingleFull:
        if (val != allVal)
        {
            PVV::Chunk::Decompress();
            PVV::Chunk::SetSingle(x, y, z, val);
        }
        break;
    }
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

#include <iostream>
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

void PVV::Chunk::Compile(uint32_t *inds_p)
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        auto vxl_u8_sh = vxl_u8 + 1;
        auto visMask = vismasks;
        auto cur_col_p = visMask + ChunkSide;
        auto inds_orig = inds_p;

        if (border_voxel_cnt == BorderVoxelLen) //Completely surrounded
            return;
        if (set_voxel_cnt == ChunkLen) //Chunk is full and neighbors are full too, chunk is invisible
            return;
        if (set_voxel_cnt == 0)
            return;

        for (uint32_t x = 1 << 11; x < (ChunkSide - 1) << 11; x += 1 << 11)
        {
            auto left_col = *cur_col_p++;
            auto cur_col_orig = *cur_col_p++;
            auto region = x / RegionSize;

            if (regional_voxel_cnt[region] > 0)
                for (uint32_t z = 1 << 6; z < (ChunkSide - 1) << 6; z += 1 << 6)
                {
                    auto right_col = *cur_col_p++;

                    if (cur_col_orig != 0)
                    {
                        auto top_col = *(cur_col_p - ChunkSide - 2); //top_col_p++;
                        auto btm_col = *(cur_col_p + ChunkSide - 2); //btm_col_p++;
                        auto xz = z | x;

#define MESH_LOOP(col, face)                                                       \
    {                                                                              \
        if (col != 0)                                                              \
        {                                                                          \
            uint32_t start_fidx = xz | _tzcnt_u64(col);                            \
            uint32_t fidx = start_fidx;                                            \
            uint8_t cur_mat = vxl_u8_sh[fidx];                                     \
            uint32_t next_fidx;                                                    \
            uint8_t next_mat;                                                      \
            uint64_t next_col;                                                     \
            do                                                                     \
            {                                                                      \
                next_col = _blsr_u64(col);                                         \
                next_fidx = xz | _tzcnt_u64(next_col);                             \
                next_mat = vxl_u8_sh[next_fidx];                                   \
                if ((cur_mat != next_mat) | ((fidx + 1) != next_fidx))             \
                {                                                                  \
                    inds_p = packFaces_c(start_fidx, fidx, cur_mat, inds_p, face); \
                    start_fidx = next_fidx;                                        \
                }                                                                  \
                col = next_col;                                                    \
                fidx = next_fidx;                                                  \
                cur_mat = next_mat;                                                \
            } while (col != 0);                                                    \
        }                                                                          \
    }

#define MESH_DIRECT_LOOP(col, face)                              \
    while (col != 0)                                             \
    {                                                            \
        uint32_t fidx = xz | _tzcnt_u64(col);                    \
        col = _blsr_u64(col);                                    \
        inds_p = buildFace(inds_p, fidx, vxl_u8_sh[fidx], face); \
    }

                        //RLE encode the current sequence of 64 voxels, use them as hints for further speeding up the face build
                        //

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
                cur_col_p += ChunkSide - 2;
        }

        break;
    }
    case ChunkCodingScheme::SingleFull:
    {
        //full cube, separate render path
        break;
    }
    }
}

uint32_t PVV::Chunk::GetCompiledLen()
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        auto visMask = vismasks;
        auto cur_col_p = visMask + ChunkSide;
        uint32_t expectedLen = 0;

        if (border_voxel_cnt == BorderVoxelLen)
            return 0;
        if (set_voxel_cnt == ChunkLen)
            return 0;
        if (set_voxel_cnt == 0)
            return 0;

        for (uint32_t x = 1 << 11; x < (ChunkSide - 1) << 11; x += 1 << 11)
        {
            auto left_col = *cur_col_p++;
            auto cur_col_orig = *cur_col_p++;
            auto region = x / RegionSize;

            if (regional_voxel_cnt[region] > 0)
                for (uint32_t z = 0; z < (ChunkSide - 2); z++)
                {
                    auto right_col = *cur_col_p++;

                    if (cur_col_orig != 0)
                    {
                        auto top_col = *(cur_col_p - ChunkSide - 2); //top_col_p++;
                        auto btm_col = *(cur_col_p + ChunkSide - 2); //btm_col_p++;

                        uint64_t cur_col = __andn_u64(cur_col_orig << 1, cur_col_orig) >> 1;
                        expectedLen += __popcntq(cur_col);

                        uint64_t cur_col2 = __andn_u64((int64_t)cur_col_orig >> 1, cur_col_orig) >> 1;
                        expectedLen += __popcntq(cur_col2);

                        uint64_t top_vis = __andn_u64(top_col, cur_col_orig) >> 1; //(top_col ^ cur_col) & cur_col;
                        expectedLen += __popcntq(top_vis);

                        uint64_t btm_vis = __andn_u64(btm_col, cur_col_orig) >> 1; //(btm_col ^ cur_col) & cur_col;
                        expectedLen += __popcntq(btm_vis);

                        uint64_t left_vis = __andn_u64(left_col, cur_col_orig) >> 1; //(left_col ^ cur_col) & cur_col;
                        expectedLen += __popcntq(left_vis);

                        uint64_t right_vis = __andn_u64(right_col, cur_col_orig) >> 1; //(right_col ^ cur_col) & cur_col;
                        expectedLen += __popcntq(right_vis);
                    }

                    left_col = cur_col_orig;
                    cur_col_orig = right_col;
                }
            else
                cur_col_p += ChunkSide - 2;
        }
        return expectedLen * 6;
    }
    case ChunkCodingScheme::SingleFull:
    {
        //full cube, separate render path
        return 6 * 6;
    }
    }
    return -1;
}

void *PVV::Chunk::GetRawData()
{
    return (void *)vxl_u8;
}

uint32_t PVV::Chunk::GetVoxelCount()
{
    return set_voxel_cnt;
}

void PVV::Chunk::Decompress()
{
    if (codingScheme == ChunkCodingScheme::SingleFull)
    {
        vismasks = new uint64_t[ChunkSide * ChunkSide];
        memset(vismasks, (allVal != -1) ? 0xff : 0x00, ChunkSide * ChunkSide * sizeof(uint64_t));

        vxl_u8 = mem_parent->AllocChunkBlock();
        if (allVal != -1)
        {
            border_voxel_cnt = BorderVoxelLen;
            set_voxel_cnt = ChunkLen;
            for (int i = 0; i < RegionCount; i++)
            {
                regional_voxel_cnt[i] = RegionSize;
                mem_parent->CommitChunkRegion(vxl_u8, i);
            }
            memset(vxl_u8, allVal, ChunkLen);
        }
        else
        {
            border_voxel_cnt = 0;
            set_voxel_cnt = 0;
            for (int i = 0; i < RegionCount; i++)
                regional_voxel_cnt[i] = 0;
        }
        codingScheme = ChunkCodingScheme::ByteRep;
    }
}

void PVV::Chunk::SetPosition(glm::ivec3 &pos)
{
    position = pos;
}

glm::ivec3 &PVV::Chunk::GetPosition()
{
    return position;
}

PVV::ChunkCodingScheme PVV::Chunk::GetCodingScheme()
{
    return codingScheme;
}

PVV::Chunk::~Chunk()
{
    if (vxl_u8 != nullptr)
        mem_parent->FreeChunkBlock(vxl_u8);

    if (vismasks != nullptr)
        delete[] vismasks;
}