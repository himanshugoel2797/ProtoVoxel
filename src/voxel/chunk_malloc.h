#pragma once
#include <stdint.h>
#include <stack>

namespace ProtoVoxel::Voxel
{
    class Chunk;
    class ChunkMalloc
    {
    private:
        uint8_t *data_base;
        std::stack<uint32_t> block_presence_bmp;

    public:
        ChunkMalloc();
        ~ChunkMalloc();

        void Initialize();

        uint8_t *AllocChunkBlock();
        void FreeChunkBlock(uint8_t *block);

        uint8_t *CommitChunkRegion(uint8_t *block, int region_idx);
        void DecommitChunkBlock(uint8_t *block, int region_idx);
    };
} // namespace ProtoVoxel::Voxel