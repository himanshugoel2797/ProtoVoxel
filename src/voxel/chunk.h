#pragma once

#include <stdint.h>

namespace ProtoVoxel::Voxel {
    enum class ChunkCodingScheme {
        None,       //No compression
        LinearRLE,  //plain RLE
        SingleFull, //Single block type for the entire chunk
        LayerFull,  //Single block type per layer
        CompactLUT0,//16 block type lookup table
    };

    class Chunk {
    public:
        static const int ChunkSide = 16;
        static const int ChunkLayers = 32;
    private:
        ChunkCodingScheme codingScheme;
        uint8_t allVal;
        uint8_t *voxels;
    public:
        Chunk();
        void SetAll(uint8_t val);
        void SetSingle(uint8_t x, uint8_t y, uint8_t z, uint8_t val);
        void Compress();
        void Decompress();
        ~Chunk();
    };
} // namespace ProtoVoxel::Voxel