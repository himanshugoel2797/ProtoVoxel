#include "chunk.h"
#include "mortoncode.h"
#include <string.h>
//#include <x86intrin.h>

namespace PVV = ProtoVoxel::Voxel;

PVV::Chunk::Chunk(ChunkMalloc *mem_parent)
{
    this->mem_parent = mem_parent;
    vxl_u8 = nullptr;
    vismasks = nullptr;
    codingScheme = ChunkCodingScheme::SingleFull;
    allVal = 0;
    set_voxel_cnt = 0;
}

void PVV::Chunk::SetAll(uint16_t val)
{
    codingScheme = ChunkCodingScheme::SingleFull;
    allVal = val;
    if (allVal != 0)
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

void PVV::Chunk::SetSingle(uint8_t x, uint8_t y, uint8_t z, uint16_t val)
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        uint32_t b_idx = EncodeXZ(x, z);
        auto vmask = vismasks[b_idx];
        auto mask = (1 << y);
        auto region = x / RegionLayerCount;

        bool add_voxel = ((vmask & mask) == 0 && val != 0);
        bool rm_voxel = ((vmask & mask) != 0 && val == 0);
        vismasks[b_idx] = (vmask & ~mask) | (-(val != 0) & mask); //Set the visibility bit appropriately
        if (add_voxel)
        {
            if (regional_voxel_cnt[region] == 0)
                mem_parent->CommitChunkRegion(vxl_u8, region);
            regional_voxel_cnt[region]++;
            set_voxel_cnt++;
        }
        uint32_t idx = EncodeXZ_Y(b_idx, y);
        vxl_u8[idx] = val;
        if (rm_voxel)
        {
            regional_voxel_cnt[region]--;
            if (regional_voxel_cnt[region] == 0)
                mem_parent->DecommitChunkBlock(vxl_u8, region);
            set_voxel_cnt--;
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

const uint32_t backFace = 0;
const uint32_t frontFace = 1 << 29;
const uint32_t topFace = 2 << 29;
const uint32_t btmFace = 3 << 29;
const uint32_t leftFace = 4 << 29;
const uint32_t rightFace = 5 << 29;

static inline uint32_t *buildFace(uint32_t *inds, uint32_t xz, uint32_t y, uint8_t *vxl_data, uint32_t face)
{
    auto xyz = xz | y;
    auto mat = vxl_data[xyz];

    uint32_t x_axis;
    uint32_t y_axis;
    uint32_t cmn_axis;

    switch (face)
    {
    case backFace:
        cmn_axis = (1 << 19) | face;
        x_axis = (1 << 24) | cmn_axis; //x
        y_axis = (1 << 14) | cmn_axis; //y
        break;
    case frontFace:
        cmn_axis = 0 | face;
        x_axis = (1 << 24) | cmn_axis; //x
        y_axis = (1 << 14) | cmn_axis; //y
        break;
    case topFace:
        cmn_axis = (1 << 14) | face;   //y
        x_axis = (1 << 24) | cmn_axis; //x
        y_axis = (1 << 19) | cmn_axis; //z
        break;
    case btmFace:
        cmn_axis = 0 | face;
        x_axis = (1 << 24) | cmn_axis; //x
        y_axis = (1 << 19) | cmn_axis; //z
        break;
    case leftFace:
        cmn_axis = (1 << 24) | face;   //x
        x_axis = (1 << 14) | cmn_axis; //y
        y_axis = (1 << 19) | cmn_axis; //z
        break;
    case rightFace:
        cmn_axis = 0 | face;
        x_axis = (1 << 14) | cmn_axis; //y
        y_axis = (1 << 19) | cmn_axis; //z
        break;
    }

    auto base_v = ((xyz << 14) | mat);
    auto base_v_x = base_v + x_axis;
    auto base_v_y = base_v + y_axis;
    inds[3] = inds[0] = base_v + cmn_axis;
    inds[1] = base_v_x;
    inds[5] = base_v_y;
    inds[4] = inds[2] = base_v_x | base_v_y;
    return inds + 6;
}

#if 1
#include <x86intrin.h>
#define _andn_u32(a, b, ovar) ovar = __andn_u32(a, b)

#define _blsr_u32(a, ovar) ovar = __blsr_u32(a);

#define _tzcnt_u32(ax, ovar) ovar = __tzcnt_u32(ax);
#endif

void PVV::Chunk::Compile(uint32_t *inds_p)
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        auto visMask = vismasks;
        auto cur_col_p = visMask + ChunkSide;
        auto inds_orig = inds_p;

        for (uint32_t x = 1 << 10; x < (ChunkSide - 1) << 10; x += 1 << 10)
        {
            auto left_col = *cur_col_p++;
            auto cur_col = *cur_col_p++;
            auto cur_col_orig = cur_col;
            auto region = x / RegionSize;

            if (regional_voxel_cnt[region] > 0)
                for (uint32_t z = 1 << 5; z < (ChunkSide - 1) << 5; z += 1 << 5)
                {
                    auto right_col = *cur_col_p++;

                    if (cur_col != 0)
                    {
                        auto top_col = *(cur_col_p - ChunkSide - 2); //top_col_p++;
                        auto btm_col = *(cur_col_p + ChunkSide - 2); //btm_col_p++;
                        auto xz = z | x;

                        while (cur_col != 0)
                        {
                            uint32_t fidx;
                            _tzcnt_u32(cur_col, fidx);
                            inds_p = buildFace(inds_p, xz, fidx, vxl_u8, backFace);
                            _blsr_u32(cur_col, cur_col);
                        }

                        uint32_t cur_col2;
                        _andn_u32(cur_col_orig >> 1, cur_col_orig, cur_col2);
                        while (cur_col2 != 0)
                        {
                            uint32_t fidx;
                            _tzcnt_u32(cur_col2, fidx);
                            inds_p = buildFace(inds_p, xz, fidx, vxl_u8, frontFace);
                            _blsr_u32(cur_col2, cur_col2);
                        }

                        uint32_t top_vis;
                        _andn_u32(top_col, cur_col_orig, top_vis); //(top_col ^ cur_col) & cur_col;
                        while (top_vis != 0)
                        {
                            uint32_t fidx;
                            _tzcnt_u32(top_vis, fidx);
                            inds_p = buildFace(inds_p, xz, fidx, vxl_u8, topFace); //append face
                            _blsr_u32(top_vis, top_vis);
                        }

                        uint32_t btm_vis;
                        _andn_u32(btm_col, cur_col_orig, btm_vis); //(btm_col ^ cur_col) & cur_col;
                        while (btm_vis != 0)
                        {
                            uint32_t fidx;
                            _tzcnt_u32(btm_vis, fidx);
                            inds_p = buildFace(inds_p, xz, fidx, vxl_u8, btmFace); //append face
                            _blsr_u32(btm_vis, btm_vis);
                        }

                        uint32_t left_vis;
                        _andn_u32(left_col, cur_col_orig, left_vis); //(left_col ^ cur_col) & cur_col;
                        while (left_vis != 0)
                        {
                            uint32_t fidx;
                            _tzcnt_u32(left_vis, fidx);
                            inds_p = buildFace(inds_p, xz, fidx, vxl_u8, leftFace); //append face
                            _blsr_u32(left_vis, left_vis);
                        }

                        uint32_t right_vis;
                        _andn_u32(right_col, cur_col_orig, right_vis); //(right_col ^ cur_col) & cur_col;
                        while (right_vis != 0)
                        {
                            uint32_t fidx;
                            _tzcnt_u32(right_vis, fidx);
                            inds_p = buildFace(inds_p, xz, fidx, vxl_u8, rightFace); //append face
                            _blsr_u32(right_vis, right_vis);
                        }
                    }

                    left_col = cur_col_orig;
                    cur_col_orig = cur_col = right_col;
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

        for (uint32_t x = 0; x < (ChunkSide - 2); x++)
        {
            auto left_col = *cur_col_p++;
            auto cur_col_orig = *cur_col_p++;
            auto region = x / RegionLayerCount;

            if (regional_voxel_cnt[region] > 0)
                for (uint32_t z = 0; z < (ChunkSide - 2); z++)
                {
                    auto right_col = *cur_col_p++;

                    if (cur_col_orig != 0)
                    {
                        auto top_col = *(cur_col_p - ChunkSide - 2); //top_col_p++;
                        auto btm_col = *(cur_col_p + ChunkSide - 2); //btm_col_p++;
                        expectedLen += __popcntd(cur_col_orig);

                        uint32_t cur_col2;
                        _andn_u32(cur_col_orig >> 1, cur_col_orig, cur_col2);
                        expectedLen += __popcntd(cur_col2);

                        uint32_t top_vis;
                        _andn_u32(top_col, cur_col_orig, top_vis); //(top_col ^ cur_col) & cur_col;
                        expectedLen += __popcntd(top_vis);

                        uint32_t btm_vis;
                        _andn_u32(btm_col, cur_col_orig, btm_vis); //(btm_col ^ cur_col) & cur_col;
                        expectedLen += __popcntd(btm_vis);

                        uint32_t left_vis;
                        _andn_u32(left_col, cur_col_orig, left_vis); //(left_col ^ cur_col) & cur_col;
                        expectedLen += __popcntd(left_vis);

                        uint32_t right_vis;
                        _andn_u32(right_col, cur_col_orig, right_vis); //(right_col ^ cur_col) & cur_col;
                        expectedLen += __popcntd(right_vis);
                    }

                    left_col = cur_col_orig;
                    cur_col_orig = right_col;
                }
            else
                cur_col_p += ChunkSide - 2;
        }

        return expectedLen * 6;
        break;
    }
    case ChunkCodingScheme::SingleFull:
    {
        //full cube, separate render path
        break;
    }
    }
    return -1;
}

void *PVV::Chunk::GetRawData()
{
    return (void *)vxl_u8;
}

uint16_t PVV::Chunk::GetVoxelCount()
{
    return set_voxel_cnt;
}

void PVV::Chunk::Decompress()
{
    if (codingScheme == ChunkCodingScheme::SingleFull)
    {
        vismasks = new uint32_t[ChunkSide * ChunkSide];
        memset(vismasks, (allVal != 0) ? 0xff : 0x00, ChunkSide * ChunkSide * sizeof(uint32_t));

        vxl_u8 = mem_parent->AllocChunkBlock();
        if (allVal != 0)
        {
            set_voxel_cnt = ChunkLen;
            for (int i = 0; i < RegionCount; i++)
            {
                regional_voxel_cnt[i] = 4096;
                mem_parent->CommitChunkRegion(vxl_u8, i);
            }
            memset(vxl_u8, allVal, ChunkLen);
        }
        else
        {
            set_voxel_cnt = 0;
            for (int i = 0; i < RegionCount; i++)
                regional_voxel_cnt[i] = 0;
        }
        codingScheme = ChunkCodingScheme::ByteRep;
    }
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