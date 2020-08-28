#pragma once

#include "glm/glm.hpp"
#include <stdint.h>
#include <vector>

namespace ProtoVoxel::Voxel
{
    enum class ChunkCodingScheme
    {
        SingleFull, //Single block type for the entire chunk
        ByteRep,    //8 bit translation table
    };

    enum class ChunkStatus {
        None,
        MeshUpdatePending,
    };

    class ChunkUpdater;

    class Chunk
    {
    public:
        static const int ChunkSide = 32;
        static const int ChunkLayers = 64;
        static const int ChunkLen = ChunkSide * ChunkSide * ChunkLayers;
        static const int InternalVoxelLen = (ChunkSide - 2) * (ChunkSide - 2) * (ChunkLayers - 1);
        static const int BorderVoxelLen = (ChunkSide * ChunkSide) * 2 + (ChunkSide * ChunkLayers) * 4;
        static const int RegionSize = 4096;
        static const int RegionCount = ChunkLen / RegionSize; //(32 * 32 * 32 / 4096)
        static const int RegionLayerCount = ChunkLayers / RegionCount;
        static inline uint32_t Encode(uint8_t x, uint8_t y, uint8_t z)
        {
            return y | ((uint32_t)z << 6) | ((uint32_t)x << 11);
        }

        static inline uint32_t EncodeXZ(uint8_t x, uint8_t z)
        {
            return ((uint32_t)z) | ((uint32_t)x << 5);
        }

        static inline uint32_t EncodeXZ_Y(uint32_t xz, uint8_t y)
        {
            return y | (xz << 6);
        }

    private:
        uint32_t set_voxel_cnt;
        uint32_t border_voxel_cnt;
        uint16_t regional_voxel_cnt[RegionCount];
        
        glm::ivec3 position;
        uint8_t *compressed_data;
        uint32_t compressed_len;

        ChunkStatus status;
        uint32_t loopback_cnt;
        uint32_t* mesh_area_ptr;
        uint32_t mesh_area_len;

        friend class ChunkUpdater;

    public:
        Chunk();
        ~Chunk();

        void Initialize();

        void SetPosition(glm::ivec3 &pos);
        glm::ivec3 &GetPosition();
    };
} // namespace ProtoVoxel::Voxel