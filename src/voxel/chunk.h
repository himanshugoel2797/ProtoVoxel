#pragma once

#include <stdint.h>
#include <vector>
#include "chunk_malloc.h"

namespace ProtoVoxel::Voxel
{
    enum class ChunkCodingScheme
    {
        SingleFull, //Single block type for the entire chunk
        ByteRep,    //8 bit translation table
    };

    class Chunk
    {
    public:
        static const int ChunkSide = 32;
        static const int ChunkLayers = 32;
        //each entry is: 15 bit positional code + 1 spare bit + 3 bit visibility mask + 13 bit block ID
        //gpu can process these directly into meshlets
        //cpu can use generic compression schemes on top before archiving
        static const int ChunkLen = ChunkSide * ChunkSide * ChunkLayers;
        static const int RegionCount = 8; //(32 * 32 * 32 / 4096)
        static const int RegionSize = ChunkLen / RegionCount;
        static const int RegionLayerCount = ChunkLayers / RegionCount;

    private:
        ChunkMalloc *mem_parent;
        ChunkCodingScheme codingScheme;
        uint16_t allVal;
        uint16_t set_voxel_cnt;
        uint16_t regional_voxel_cnt[RegionCount];
        uint32_t *vismasks;
        uint8_t *vxl_u8;

        static inline uint32_t Encode(uint8_t x, uint8_t y, uint8_t z)
        {
            return y | ((uint32_t)z << 5) | ((uint32_t)x << 10);
        }

        static inline uint32_t EncodeXZ(uint8_t x, uint8_t z)
        {
            return ((uint32_t)z) | ((uint32_t)x << 5);
        }

        static inline uint32_t EncodeXZ_Y(uint32_t xz, uint8_t y)
        {
            return y | (xz << 5);
        }

    public:
        Chunk(ChunkMalloc *mem_parent);
        ~Chunk();

        ChunkCodingScheme GetCodingScheme();

        void SetAll(uint16_t val);
        void SetSingle(uint8_t x, uint8_t y, uint8_t z, uint16_t val);

        void Compile(uint32_t *inds_p);
        uint32_t GetCompiledLen();

        void *GetRawData();
        uint16_t GetVoxelCount();
        void Expand();
        void Decompress();
    };
} // namespace ProtoVoxel::Voxel