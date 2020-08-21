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

    buckets[0] = new uint32_t[ChunkLen * 6 * 6];
    buckets[1] = new uint32_t[ChunkLen * 6 * 6];
    buckets[2] = new uint32_t[ChunkLen * 6 * 6];
    buckets[3] = new uint32_t[ChunkLen * 6 * 6];
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

static inline uint32_t *buildFace_old(uint32_t *inds, uint32_t xz, uint32_t y, uint8_t *vxl_data, uint32_t face)
{
    auto xyz = xz | y;
    auto mat = vxl_data[xyz + 1];

    uint32_t x_axis;
    uint32_t y_axis;
    uint32_t cmn_axis;

    switch (face)
    {
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
    }

    auto base_v = ((xyz << 8) | mat | face);
    auto base_v_cmn = base_v + cmn_axis;
    auto base_v_x = base_v_cmn + x_axis;
    auto base_v_y = base_v_cmn + y_axis;
    auto base_v_x_y = base_v_cmn + x_axis + y_axis;

    switch (face)
    {
    case btmFace:
        inds[3] = inds[0] = base_v_cmn;
        inds[2] = base_v_x;
        inds[4] = base_v_y;
        inds[5] = inds[1] = base_v_x_y;
        break;
    case frontFace:
        inds[3] = inds[0] = base_v_cmn;
        inds[1] = base_v_x;
        inds[5] = base_v_y;
        inds[4] = inds[2] = base_v_x_y;
        break;
    }
    return inds + 6;
}

static inline uint32_t *buildFace(uint32_t *inds, uint32_t x, uint32_t y, uint32_t z, uint8_t *vxl_data, uint32_t face)
{
    auto xyz = x | z | y;
    auto mat = vxl_data[xyz + 1];
    z >>= 6;
    x >>= 11;

    uint32_t x_axis;
    uint32_t y_axis;
    uint32_t cmn_axis;

    switch (face)
    {
    case leftFace:
        cmn_axis = z + 1;
        x_axis = x; //x
        y_axis = y; //y
        break;
    case rightFace:
        cmn_axis = z;
        x_axis = x; //x
        y_axis = y; //y
        break;
    case topFace:
        cmn_axis = x + 1; //x
        x_axis = y;       //y
        y_axis = z;       //z
        break;
    case btmFace:
        cmn_axis = x;
        x_axis = y; //y
        y_axis = z; //z
        break;
    }

    //for bucketing phase, store 'flattened' representation that lends itself to faster compression
    auto base_v_cmn = (cmn_axis << 24 | x_axis << 16 | y_axis << 8 | mat);
    auto base_v_x = base_v_cmn + (1 << 16);
    auto base_v_y = base_v_cmn + (1 << 8);
    auto base_v_x_y = base_v_cmn + ((1 << 16) | (1 << 8));

    //std::cout << "base_v_cmn: " << base_v_x << "x: " << x << " y: " << y << " z: " << z << std::endl;
    switch (face)
    {
    case rightFace:
    case btmFace:
        inds[3] = inds[0] = base_v_cmn;
        inds[2] = base_v_x;
        inds[4] = base_v_y;
        inds[5] = inds[1] = base_v_x_y;
        break;
    case topFace:
    case leftFace:
        inds[3] = inds[0] = base_v_cmn;
        inds[1] = base_v_x;
        inds[5] = base_v_y;
        inds[4] = inds[2] = base_v_x_y;
        break;
    }
    return inds + 6;
}

static inline uint32_t reencode(uint32_t val, uint32_t face)
{
    uint32_t x, y, z;
    switch (face)
    {
    case leftFace:
    case rightFace:
        z = (val >> 24);
        x = (val >> 16) & 0x1f;
        y = (val >> 8) & 0x3f;
        break;
    case backFace:
    case frontFace:
        z = (val >> 8) & 0x1f;
        x = (val >> 16) & 0x1f;
        y = (val >> 24);
        break;
    case topFace:
    case btmFace:
        z = (val >> 8) & 0x1f;
        x = (val >> 24);
        y = (val >> 16) & 0x3f;
        break;
    }
    //std::cout << "x: " << x << " y: " << y << " z: " << z << std::endl;
    return (x << 19) | (y << 8) | (z << 14) | (val & 0xff);
}

static inline uint32_t *packFaces(uint32_t *start, uint32_t *end, uint32_t *inds, uint32_t face)
{
    //read start[1] as the common corner, read each +6, compute desired vertex and emit collapsed face
    uint32_t cur_crn = start[1];
    uint32_t cur_start_base = 0;
    uint32_t crn_cntr = 1;
    uint32_t len = end - start;

    uint32_t addend;
    switch (face)
    {
    case topFace:
    case btmFace:
        addend = (1 << 16);
        break;
    case leftFace:
    case rightFace:
        addend = (1 << 8);
        break;
    }

    int idx = crn_cntr * 6 + 1;
    while (idx < len)
    {
        if (start[idx] != (cur_crn + addend))
        {
            //std::cout << std::hex << "start[idx] = " << start[idx] << " cur_crn + addres = " << cur_crn + addend << std::endl;
            switch (face)
            {
            case rightFace:
                inds[0] = inds[3] = reencode(start[cur_start_base], face);
                inds[1] = inds[5] = reencode(start[(crn_cntr - 1) * 6 + 1], face);
                inds[2] = reencode(start[cur_start_base + 2], face);
                inds[4] = reencode(start[(crn_cntr - 1) * 6 + 4], face);
                break;
            case btmFace:
                inds[0] = inds[3] = reencode(start[cur_start_base], face);
                inds[1] = inds[5] = reencode(start[(crn_cntr - 1) * 6 + 1], face);
                inds[2] = reencode(start[(crn_cntr - 1) * 6 + 2], face);
                inds[4] = reencode(start[cur_start_base + 4], face);
                break;
            case topFace:
                inds[0] = inds[3] = reencode(start[cur_start_base], face);
                inds[4] = inds[2] = reencode(start[(crn_cntr - 1) * 6 + 2], face);
                inds[1] = reencode(start[(crn_cntr - 1) * 6 + 1], face);
                inds[5] = reencode(start[cur_start_base + 5], face);
                break;
            case leftFace:
                inds[0] = inds[3] = reencode(start[cur_start_base], face);
                inds[4] = inds[2] = reencode(start[(crn_cntr - 1) * 6 + 2], face);
                inds[1] = reencode(start[cur_start_base + 1], face);
                inds[5] = reencode(start[(crn_cntr - 1) * 6 + 5], face);
                break;
            }
            //emit and restart process
            inds += 6;

            cur_start_base = crn_cntr * 6;
        }

        crn_cntr++;
        cur_crn = start[idx];
        idx = crn_cntr * 6 + 1;
    }
    if (cur_start_base < len)
    {
        switch (face)
        {
        case rightFace:
            inds[0] = inds[3] = reencode(start[cur_start_base], face);
            inds[1] = inds[5] = reencode(start[(crn_cntr - 1) * 6 + 1], face);
            inds[2] = reencode(start[cur_start_base + 2], face);
            inds[4] = reencode(start[(crn_cntr - 1) * 6 + 4], face);
            break;
        case btmFace:
            inds[0] = inds[3] = reencode(start[cur_start_base], face);
            inds[1] = inds[5] = reencode(start[(crn_cntr - 1) * 6 + 1], face);
            inds[2] = reencode(start[(crn_cntr - 1) * 6 + 2], face);
            inds[4] = reencode(start[cur_start_base + 4], face);
            break;
        case topFace:
            inds[0] = inds[3] = reencode(start[cur_start_base], face);
            inds[4] = inds[2] = reencode(start[(crn_cntr - 1) * 6 + 2], face);
            inds[1] = reencode(start[(crn_cntr - 1) * 6 + 1], face);
            inds[5] = reencode(start[cur_start_base + 5], face);
            break;
        case leftFace:
            inds[0] = inds[3] = reencode(start[cur_start_base], face);
            inds[4] = inds[2] = reencode(start[(crn_cntr - 1) * 6 + 2], face);
            inds[1] = reencode(start[cur_start_base + 1], face);
            inds[5] = reencode(start[(crn_cntr - 1) * 6 + 5], face);
            break;
        }

        inds += 6;
    }
    return inds;
}

void PVV::Chunk::Compile(uint32_t *inds_p)
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        auto visMask = vismasks;
        auto cur_col_p = visMask + ChunkSide;
        auto inds_orig = inds_p;

        if (border_voxel_cnt == BorderVoxelLen) //Completely surrounded
            return;
        if (set_voxel_cnt == ChunkLen) //Chunk is full and neighbors are full too, chunk is invisible
            return;
        if (set_voxel_cnt == 0)
            return;

        uint32_t *bkts_p[] = {buckets[0], buckets[1], buckets[2], buckets[3]};
        memset(bkts_p[0], 0, 64 * 1024);
        memset(bkts_p[1], 0, 64 * 1024);
        memset(bkts_p[2], 0, 64 * 1024);
        memset(bkts_p[3], 0, 64 * 1024);

        uint32_t loop_cnt = 0;
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

#define MESH_LOOP(col, face, bkt)                                       \
    while (col != 0)                                                    \
    {                                                                   \
        uint32_t fidx = _tzcnt_u64(col);                                \
        bkts_p[bkt] = buildFace(bkts_p[bkt], x, fidx, z, vxl_u8, face); \
        col = _blsr_u64(col);                                           \
    }

#define MESH_DIRECT_LOOP(col, face)                             \
    while (col != 0)                                            \
    {                                                           \
        uint32_t fidx = _tzcnt_u64(col);                        \
        inds_p = buildFace_old(inds_p, xz, fidx, vxl_u8, face); \
        col = _blsr_u64(col);                                   \
    }

                        uint64_t cur_col = __andn_u64(cur_col_orig << 1, cur_col_orig) >> 1;
                        MESH_DIRECT_LOOP(cur_col, frontFace);

                        uint64_t cur_col2 = __andn_u64((int64_t)cur_col_orig >> 1, cur_col_orig) >> 1;
                        MESH_DIRECT_LOOP(cur_col2, backFace);

                        uint64_t top_vis = __andn_u64(top_col, cur_col_orig) >> 1; //(top_col ^ cur_col) & cur_col;
                        MESH_LOOP(top_vis, btmFace, 0);

                        uint64_t btm_vis = __andn_u64(btm_col, cur_col_orig) >> 1; //(btm_col ^ cur_col) & cur_col;
                        MESH_LOOP(btm_vis, topFace, 1);

                        uint64_t left_vis = __andn_u64(left_col, cur_col_orig) >> 1; //(left_col ^ cur_col) & cur_col;
                        MESH_LOOP(left_vis, rightFace, 2);

                        uint64_t right_vis = __andn_u64(right_col, cur_col_orig) >> 1; //(right_col ^ cur_col) & cur_col;
                        MESH_LOOP(right_vis, leftFace, 3);
                    }

                    left_col = cur_col_orig;
                    cur_col_orig = right_col;
                }
            else
                cur_col_p += ChunkSide - 2;
        }

        //for each bucket, merge neighboring faces by collapsing common edges
        inds_p = packFaces(buckets[0], bkts_p[0], inds_p, btmFace);
        inds_p = packFaces(buckets[1], bkts_p[1], inds_p, topFace);
        inds_p = packFaces(buckets[2], bkts_p[2], inds_p, rightFace);
        inds_p = packFaces(buckets[3], bkts_p[3], inds_p, leftFace);
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

    delete[] buckets[0];
    delete[] buckets[1];
    delete[] buckets[2];
    delete[] buckets[3];
}