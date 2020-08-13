#pragma once

#include <stdint.h>

namespace ProtoVoxel::Voxel
{
    enum class ChunkCodingScheme
    {
        SingleFull, //Single block type for the entire chunk
        ByteRep,    //3 bit mask + 5 bit translation table
        ShortRep,   //3 bit mask + 13 bit translation table
    };

    struct ChunkCoded
    {
        uint16_t x : 5;
        uint16_t y : 5;
        uint16_t z : 5;
        uint16_t full_blk : 1;
        union
        {
            struct
            {
                uint16_t v0 : 4;
                uint16_t v1 : 4;
                uint16_t v2 : 4;
                uint16_t v3 : 4;
            } u4;
            struct
            {
                uint16_t v0 : 8;
                uint16_t v1 : 8;
            } u8;
            uint16_t u16;
        } vxls;
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

    private:
        ChunkCodingScheme codingScheme;
        uint16_t allVal;
        uint16_t set_voxel_cnt;
        union
        {
            uint16_t *vxl_u16;
            uint8_t *vxl_u8;
            struct
            {
                uint8_t v0 : 4;
                uint8_t v1 : 4;
            } * vxl_u4;
        };

    public:
        Chunk();
        ~Chunk();

        ChunkCodingScheme GetCodingScheme();
        void SetAll(uint16_t val);
        void SetSingle(uint8_t x, uint8_t y, uint8_t z, uint16_t val);

        int CompiledLen();
        void Compile(struct ProtoVoxel::Voxel::ChunkCoded *coded);

        void *GetRawData();
        uint16_t GetVoxelCount();
        void Expand();
        void Decompress();
    };
} // namespace ProtoVoxel::Voxel