#pragma once

#include "glm/glm.hpp"
#include <stdint.h>
#include <vector>

namespace ProtoVoxel::PointCloud
{
    enum class ChunkStatus {
        None,
        SurfaceUpdatePending,
    };

    class ChunkUpdater;
    class ChunkJobManager;

    class Chunk
    {
    public:
        static const int ChunkSide = 32;
        static const int ChunkLen = ChunkSide * ChunkSide * ChunkSide;
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

    private:
        glm::ivec3 position;
        glm::ivec3 min_bound;
        glm::ivec3 max_bound;
        uint8_t *compressed_data;
        uint32_t compressed_len;

        ChunkStatus status;
        uint32_t loopback_cnt;
        uint32_t mesh_area_ptr;
        uint32_t mesh_area_len;

        uint32_t id;

        friend class ChunkUpdater;
        friend class ChunkJobManager;

    public:
        Chunk();
        ~Chunk();

        void Initialize(uint32_t id);

        void SetPosition(glm::ivec3 &pos);
        glm::ivec3 &GetPosition();
    };
} // namespace ProtoVoxel::Voxel