#pragma once

#include <stdint.h>
#include <vector>

namespace ProtoVoxel::Voxel
{
    class ChunkPalette
    {
    private:
        std::vector<uint16_t> palette;
        std::vector<uint16_t> refCounts;

    public:
        ChunkPalette();
        ~ChunkPalette();

        int Count();
        int Register(uint16_t val);
        int Deregister(uint16_t val);
    };
} // namespace ProtoVoxel::Voxel