#include "chunk.h"
#include "mortoncode.h"
#include <string.h>
#include <vector>

namespace PVV = ProtoVoxel::Voxel;

PVV::Chunk::Chunk()
{
    vxl_u8 = nullptr;
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
    }
}

void PVV::Chunk::SetSingle(uint8_t x, uint8_t y, uint8_t z, uint16_t val)
{
    uint32_t idx = MortonCode::Encode(x, y, z);

    uint32_t idx_n[6];
    idx_n[0] = MortonCode::Add(idx, MortonCode::OneX);
    idx_n[1] = MortonCode::Add(idx, MortonCode::OneY);
    idx_n[2] = MortonCode::Add(idx, MortonCode::OneZ);
    idx_n[3] = MortonCode::Sub(idx, MortonCode::OneX);
    idx_n[4] = MortonCode::Sub(idx, MortonCode::OneY);
    idx_n[5] = MortonCode::Sub(idx, MortonCode::OneZ);

    if (x == ChunkSide - 1)
        idx_n[0] = ChunkLen;
    if (y == ChunkSide - 1)
        idx_n[1] = ChunkLen;
    if (z == ChunkSide - 1)
        idx_n[2] = ChunkLen;
    if (x == 0)
        idx_n[3] = ChunkLen;
    if (y == 0)
        idx_n[4] = ChunkLen;
    if (z == 0)
        idx_n[5] = ChunkLen;

    uint32_t idx_n_n;
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        //TODO: Register value to palette, if the palette exceeds 32 entries, expand to ShortRep form
        //Figure out how to optimize the occupancy for gpu meshing, may just have to iterate and submit visible voxels
        bool rm_voxel = (vxl_u8[idx] != 0 && val == 0);

        if (val != 0)
        {
            //Copy over the mask if it's a new voxel
            val = (vxl_u8[idx] & 0xe0) | val;
            if (vxl_u8[idx] == 0) //Rebuild the mask if this voxel was previously invisible
            {
                if ((vxl_u8[idx_n[0]] == 0) | (vxl_u8[idx_n[3]] == 0))
                    val |= (1 << 5);
                if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[4]] == 0))
                    val |= (1 << 6);
                if ((vxl_u8[idx_n[2]] == 0) | (vxl_u8[idx_n[5]] == 0))
                    val |= (1 << 7);

                set_voxel_cnt++;

                if (x < ChunkSide - 2 && vxl_u8[idx_n[0]] != 0)
                {
                    idx_n_n = MortonCode::Add(idx_n[0], MortonCode::OneX);
                    if (vxl_u8[idx_n_n] == 0)
                        vxl_u8[idx_n[0]] |= (1 << 5);
                    else
                        vxl_u8[idx_n[0]] &= ~(1 << 5);
                }

                if (y < ChunkSide - 2 && vxl_u8[idx_n[3]] != 0)
                {
                    idx_n_n = MortonCode::Sub(idx_n[3], MortonCode::OneX);
                    if (vxl_u8[idx_n_n] == 0)
                        vxl_u8[idx_n[3]] |= (1 << 5);
                    else
                        vxl_u8[idx_n[3]] &= ~(1 << 5);
                }

                if (z < ChunkSide - 2 && vxl_u8[idx_n[1]] != 0)
                {
                    idx_n_n = MortonCode::Add(idx_n[1], MortonCode::OneY);
                    if (vxl_u8[idx_n_n] == 0)
                        vxl_u8[idx_n[1]] |= (1 << 6);
                    else
                        vxl_u8[idx_n[1]] &= ~(1 << 6);
                }

                if (x >= 2 && vxl_u8[idx_n[4]] != 0)
                {
                    idx_n_n = MortonCode::Sub(idx_n[4], MortonCode::OneY);
                    if (vxl_u8[idx_n_n] == 0)
                        vxl_u8[idx_n[4]] |= (1 << 6);
                    else
                        vxl_u8[idx_n[4]] &= ~(1 << 6);
                }

                if (y >= 2 && vxl_u8[idx_n[2]] != 0)
                {
                    idx_n_n = MortonCode::Add(idx_n[2], MortonCode::OneZ);
                    if (vxl_u8[idx_n_n] == 0)
                        vxl_u8[idx_n[2]] |= (1 << 7);
                    else
                        vxl_u8[idx_n[2]] &= ~(1 << 7);
                }

                if (z >= 2 && vxl_u8[idx_n[5]] != 0)
                {
                    idx_n_n = MortonCode::Sub(idx_n[5], MortonCode::OneZ);
                    if (vxl_u8[idx_n_n] == 0)
                        vxl_u8[idx_n[5]] |= (1 << 7);
                    else
                        vxl_u8[idx_n[5]] &= ~(1 << 7);
                }
            }
        }
        vxl_u8[idx] = val;

        //Recompute the mask for the neighbors
        if (rm_voxel)
        {
            set_voxel_cnt--;

            vxl_u8[idx_n[0]] |= ((vxl_u8[idx_n[0]] != 0) << 5);
            vxl_u8[idx_n[3]] |= ((vxl_u8[idx_n[3]] != 0) << 5);
            vxl_u8[idx_n[1]] |= ((vxl_u8[idx_n[1]] != 0) << 6);
            vxl_u8[idx_n[4]] |= ((vxl_u8[idx_n[4]] != 0) << 6);
            vxl_u8[idx_n[2]] |= ((vxl_u8[idx_n[2]] != 0) << 7);
            vxl_u8[idx_n[5]] |= ((vxl_u8[idx_n[5]] != 0) << 7);
        }
        break;
    }
    case ChunkCodingScheme::ShortRep:
    {
        //TODO: Register value to palette, if the palette exceeds 32 entries, expand to ShortRep form
        //Figure out how to optimize the occupancy for gpu meshing, may just have to iterate and submit visible voxels
        bool rm_voxel = (vxl_u16[idx] != 0 && val == 0);

        if (val != 0)
        {
            //Copy over the mask if it's a new voxel
            val = (vxl_u16[idx] & 0xe000) | val;
            if (vxl_u16[idx] == 0) //Rebuild the mask if this voxel was previously invisible
            {
                if ((vxl_u16[idx_n[0]] == 0) | (vxl_u16[idx_n[3]] == 0))
                    val |= (1 << 13);
                if ((vxl_u16[idx_n[1]] == 0) | (vxl_u16[idx_n[4]] == 0))
                    val |= (1 << 14);
                if ((vxl_u16[idx_n[2]] == 0) | (vxl_u16[idx_n[5]] == 0))
                    val |= (1 << 15);
                set_voxel_cnt++;

                if (x < ChunkSide - 2 && vxl_u16[idx_n[0]] != 0)
                {
                    idx_n_n = MortonCode::Add(idx_n[0], MortonCode::OneX);
                    if (vxl_u16[idx_n_n] == 0)
                        vxl_u16[idx_n[0]] |= (1 << 13);
                    else
                        vxl_u16[idx_n[0]] &= ~(1 << 13);
                }

                if (y < ChunkSide - 2 && vxl_u16[idx_n[3]] != 0)
                {
                    idx_n_n = MortonCode::Sub(idx_n[3], MortonCode::OneX);
                    if (vxl_u16[idx_n_n] == 0)
                        vxl_u16[idx_n[3]] |= (1 << 13);
                    else
                        vxl_u16[idx_n[3]] &= ~(1 << 13);
                }

                if (z < ChunkSide - 2 && vxl_u16[idx_n[1]] != 0)
                {
                    idx_n_n = MortonCode::Add(idx_n[1], MortonCode::OneY);
                    if (vxl_u16[idx_n_n] == 0)
                        vxl_u16[idx_n[1]] |= (1 << 14);
                    else
                        vxl_u16[idx_n[1]] &= ~(1 << 14);
                }

                if (x >= 2 && vxl_u16[idx_n[4]] != 0)
                {
                    idx_n_n = MortonCode::Sub(idx_n[4], MortonCode::OneY);
                    if (vxl_u16[idx_n_n] == 0)
                        vxl_u16[idx_n[4]] |= (1 << 14);
                    else
                        vxl_u16[idx_n[4]] &= ~(1 << 14);
                }

                if (y >= 2 && vxl_u16[idx_n[2]] != 0)
                {
                    idx_n_n = MortonCode::Add(idx_n[2], MortonCode::OneZ);
                    if (vxl_u16[idx_n_n] == 0)
                        vxl_u16[idx_n[2]] |= (1 << 15);
                    else
                        vxl_u16[idx_n[2]] &= ~(1 << 15);
                }

                if (z >= 2 && vxl_u16[idx_n[5]] != 0)
                {
                    idx_n_n = MortonCode::Sub(idx_n[5], MortonCode::OneZ);
                    if (vxl_u16[idx_n_n] == 0)
                        vxl_u16[idx_n[5]] |= (1 << 15);
                    else
                        vxl_u16[idx_n[5]] &= ~(1 << 15);
                }
            }
        }
        vxl_u16[idx] = val;

        //Recompute the mask for the neighbors
        if (rm_voxel)
        {
            set_voxel_cnt--;

            vxl_u16[idx_n[0]] |= ((vxl_u16[idx_n[0]] != 0) << 13);
            vxl_u16[idx_n[3]] |= ((vxl_u16[idx_n[3]] != 0) << 13);
            vxl_u16[idx_n[1]] |= ((vxl_u16[idx_n[1]] != 0) << 14);
            vxl_u16[idx_n[4]] |= ((vxl_u16[idx_n[4]] != 0) << 14);
            vxl_u16[idx_n[2]] |= ((vxl_u16[idx_n[2]] != 0) << 15);
            vxl_u16[idx_n[5]] |= ((vxl_u16[idx_n[5]] != 0) << 15);
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

void PVV::Chunk::SetSingleFast(uint8_t x, uint8_t y, uint8_t z, uint16_t val)
{
    uint32_t idx = MortonCode::Encode(x, y, z);
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        bool add_voxel = (vxl_u8[idx] == 0 && val != 0);
        bool rm_voxel = (vxl_u8[idx] != 0 && val == 0);
        if (add_voxel)
            set_voxel_cnt++;
        if (rm_voxel)
            set_voxel_cnt--;

        vxl_u8[idx] = val;
        break;
    }
    case ChunkCodingScheme::ShortRep:
    {
        bool add_voxel = (vxl_u16[idx] == 0 && val != 0);
        bool rm_voxel = (vxl_u16[idx] != 0 && val == 0);
        if (add_voxel)
            set_voxel_cnt++;
        if (rm_voxel)
            set_voxel_cnt--;

        vxl_u16[idx] = val;
        break;
    }
    case ChunkCodingScheme::SingleFull:
        if (val != allVal)
        {
            PVV::Chunk::Decompress();
            PVV::Chunk::SetSingleFast(x, y, z, val);
        }
        break;
    }
}

void PVV::Chunk::UpdateMasks()
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {

        for (int z = 1; z < ChunkSide - 1; z++)
        {
            uint32_t prev_y = vxl_u8[MortonCode::Encode(0, 0, z)];
            for (int y = 1; y < ChunkSide - 1; y++)
            {
                uint32_t idx = MortonCode::Encode(0, y, z);
                uint32_t idx_n[3];
                idx_n[0] = MortonCode::Add(idx, MortonCode::OneY);
                idx_n[1] = MortonCode::Add(idx, MortonCode::OneZ);
                idx_n[2] = MortonCode::Sub(idx, MortonCode::OneZ);

                auto val = vxl_u8[idx] & 0x1f;
                if (val != 0)
                {
                    val |= (1 << 5);
                    if ((vxl_u8[idx_n[0]] == 0) | (prev_y == 0))
                        val |= (1 << 6);
                    if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[2]] == 0))
                        val |= (1 << 7);
                    vxl_u8[idx] = val;
                }
                prev_y = val;
            }
        }
        for (int z = 1; z < ChunkSide - 1; z++)
        {
            uint32_t prev_y = vxl_u8[MortonCode::Encode(ChunkSide - 1, 0, z)];
            for (int y = 1; y < ChunkSide - 1; y++)
            {
                uint32_t idx = MortonCode::Encode(ChunkSide - 1, y, z);
                uint32_t idx_n[3];
                idx_n[0] = MortonCode::Add(idx, MortonCode::OneY);
                idx_n[1] = MortonCode::Add(idx, MortonCode::OneZ);
                idx_n[2] = MortonCode::Sub(idx, MortonCode::OneZ);

                auto val = vxl_u8[idx] & 0x1f;
                if (val != 0)
                {
                    val |= (1 << 5);
                    if ((vxl_u8[idx_n[0]] == 0) | (prev_y == 0))
                        val |= (1 << 6);
                    if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[2]] == 0))
                        val |= (1 << 7);
                    vxl_u8[idx] = val;
                }
                prev_y = val;
            }
        }
        for (int z = 1; z < ChunkSide - 1; z++)
        {
            uint32_t prev_y = vxl_u8[MortonCode::Encode(0, 0, z)];
            for (int y = 1; y < ChunkSide - 1; y++)
            {
                uint32_t idx = MortonCode::Encode(y, 0, z);
                uint32_t idx_n[3];
                idx_n[0] = MortonCode::Add(idx, MortonCode::OneX);
                idx_n[1] = MortonCode::Add(idx, MortonCode::OneZ);
                idx_n[3] = MortonCode::Sub(idx, MortonCode::OneZ);

                auto val = vxl_u8[idx] & 0x1f;
                if (val != 0)
                {
                    val |= (1 << 6);
                    if ((vxl_u8[idx_n[0]] == 0) | (prev_y == 0))
                        val |= (1 << 5);
                    if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[2]] == 0))
                        val |= (1 << 7);
                    vxl_u8[idx] = val;
                }
                prev_y = val;
            }
        }
        for (int z = 1; z < ChunkSide - 1; z++)
        {
            uint32_t prev_y = vxl_u8[MortonCode::Encode(0, ChunkSide - 1, z)];
            for (int y = 1; y < ChunkSide - 1; y++)
            {
                uint32_t idx = MortonCode::Encode(y, ChunkSide - 1, z);
                uint32_t idx_n[3];
                idx_n[0] = MortonCode::Add(idx, MortonCode::OneX);
                idx_n[1] = MortonCode::Add(idx, MortonCode::OneZ);
                idx_n[2] = MortonCode::Sub(idx, MortonCode::OneZ);

                auto val = vxl_u8[idx] & 0x1f;
                if (val != 0)
                {
                    val |= (1 << 6);
                    if ((vxl_u8[idx_n[0]] == 0) | (prev_y == 0))
                        val |= (1 << 5);
                    if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[2]] == 0))
                        val |= (1 << 7);
                    vxl_u8[idx] = val;
                }
                prev_y = val;
            }
        }
        for (int z = 1; z < ChunkSide - 1; z++)
        {
            uint32_t prev_y = vxl_u8[MortonCode::Encode(0, z, 0)];
            for (int y = 1; y < ChunkSide - 1; y++)
            {
                uint32_t idx = MortonCode::Encode(y, z, 0);
                uint32_t idx_n[3];
                idx_n[0] = MortonCode::Add(idx, MortonCode::OneX);
                idx_n[1] = MortonCode::Add(idx, MortonCode::OneY);
                idx_n[2] = MortonCode::Sub(idx, MortonCode::OneY);

                auto val = vxl_u8[idx] & 0x1f;
                if (val != 0)
                {
                    val |= (1 << 7);
                    if ((vxl_u8[idx_n[0]] == 0) | (prev_y == 0))
                        val |= (1 << 5);
                    if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[2]] == 0))
                        val |= (1 << 6);
                    vxl_u8[idx] = val;
                }
                prev_y = val;
            }
        }
        for (int z = 1; z < ChunkSide - 1; z++)
        {
            uint32_t prev_y = vxl_u8[MortonCode::Encode(0, z, ChunkSide - 1)];
            for (int y = 1; y < ChunkSide - 1; y++)
            {
                uint32_t idx = MortonCode::Encode(y, z, ChunkSide - 1);
                uint32_t idx_n[3];
                idx_n[0] = MortonCode::Add(idx, MortonCode::OneX);
                idx_n[1] = MortonCode::Add(idx, MortonCode::OneY);
                idx_n[2] = MortonCode::Sub(idx, MortonCode::OneY);

                auto val = vxl_u8[idx] & 0x1f;
                if (val != 0)
                {
                    val |= (1 << 7);
                    if ((vxl_u8[idx_n[0]] == 0) | (prev_y == 0))
                        val |= (1 << 5);
                    if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[2]] == 0))
                        val |= (1 << 6);
                    vxl_u8[idx] = val;
                }
                prev_y = val;
            }
        }

        for (int z = 1; z < ChunkSide - 1; z++)
            for (int y = 1; y < ChunkSide - 1; y++)
            {
                uint32_t prev_x = vxl_u8[MortonCode::Encode(0, y, z)];
                for (int x = 1; x < ChunkSide - 1; x++)
                {
                    uint32_t idx = MortonCode::Encode(x, y, z);

                    uint32_t idx_n[5];
                    idx_n[0] = MortonCode::Add(idx, MortonCode::OneX);
                    idx_n[1] = MortonCode::Add(idx, MortonCode::OneY);
                    idx_n[2] = MortonCode::Add(idx, MortonCode::OneZ);
                    idx_n[3] = MortonCode::Sub(idx, MortonCode::OneY);
                    idx_n[4] = MortonCode::Sub(idx, MortonCode::OneZ);

                    auto val = vxl_u8[idx] & 0x1f;
                    if (val != 0)
                    {
                        if ((vxl_u8[idx_n[0]] == 0) | (prev_x == 0))
                            val |= (1 << 5);
                        if ((vxl_u8[idx_n[1]] == 0) | (vxl_u8[idx_n[3]] == 0))
                            val |= (1 << 6);
                        if ((vxl_u8[idx_n[2]] == 0) | (vxl_u8[idx_n[4]] == 0))
                            val |= (1 << 7);
                        vxl_u8[idx] = val;
                    }
                    prev_x = val;
                }
            }
    }
    break;
    case ChunkCodingScheme::ShortRep:
    {
    }
    break;
    }
}

int PVV::Chunk::CompiledLen()
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        //2 voxels per location
        //if the entire chunk is full, submit the chunk directly for rendering as a large cube
        if (set_voxel_cnt != ChunkLen)
        {
            int ent_cnt = 0;
            for (int i = 0; i < ChunkLen / 2; i++)
                if (vxl_u16[i])
                    ent_cnt++;
            return ent_cnt;
        }
        return 0;
        break;
    }
    case ChunkCodingScheme::ShortRep:
    {
        //1 voxel per location
        //if the entire chunk is full, submit the chunk directly for rendering as a large cube
        if (set_voxel_cnt != ChunkLen)
        {
            int ent_cnt = 0;
            for (int i = 0; i < ChunkLen; i++)
                if (vxl_u16[i])
                    ent_cnt++;
            return ent_cnt;
        }
        return 0;
        break;
    }
    case ChunkCodingScheme::SingleFull:
    {
        //full cube, separate render path
        return 1;
        break;
    }
    }

    return 0;
}

void PVV::Chunk::Compile(struct PVV::ChunkCoded *coded)
{
    switch (codingScheme)
    {
    case ChunkCodingScheme::ByteRep:
    {
        //2 voxels per location
        //if the entire chunk is full, submit the chunk directly for rendering as a large cube
        if (set_voxel_cnt != ChunkLen)
        {
            int idx = 0;
            for (uint32_t i = 0; i < ChunkLen; i += 2)
                if (vxl_u16[i / 2])
                {
                    coded[idx].x = MortonCode::DecodeX(i);
                    coded[idx].y = MortonCode::DecodeY(i);
                    coded[idx].z = MortonCode::DecodeZ(i);
                    coded[idx].full_blk = 0;
                    coded[idx].vxls.u16 = vxl_u16[i / 2];
                    idx++;
                }
        }
        break;
    }
    case ChunkCodingScheme::ShortRep:
    {
        //1 voxel per location
        //if the entire chunk is full, submit the chunk directly for rendering as a large cube
        if (set_voxel_cnt != ChunkLen)
        {
            int idx = 0;
            for (uint32_t i = 0; i < ChunkLen; i++)
                if (vxl_u16[i])
                {
                    coded[idx].x = MortonCode::DecodeX(i);
                    coded[idx].y = MortonCode::DecodeY(i);
                    coded[idx].z = MortonCode::DecodeZ(i);
                    coded[idx].full_blk = 0;
                    coded[idx].vxls.u16 = vxl_u16[i];
                    idx++;
                }
        }
        break;
    }
    case ChunkCodingScheme::SingleFull:
    {
        //full cube, separate render path
        {
            coded->x = 0;
            coded->y = 0;
            coded->z = 0;
            coded->full_blk = 1;
            coded->vxls.u16 = allVal;
        }
        break;
    }
    }
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
        vxl_u8 = new uint8_t[ChunkLen + 1];
        memset(vxl_u8, allVal, ChunkLen);
        vxl_u8[ChunkLen] = 0x00;
        set_voxel_cnt = ChunkLen;
        codingScheme = ChunkCodingScheme::ByteRep;
    }
}

PVV::ChunkCodingScheme PVV::Chunk::GetCodingScheme()
{
    return codingScheme;
}

PVV::Chunk::~Chunk()
{
    if (vxl_u16 != nullptr)
        delete[] vxl_u16;
}