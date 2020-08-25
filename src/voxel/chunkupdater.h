#pragma once

#include "chunk.h"
#include <stdint.h>

namespace ProtoVoxel::Voxel
{
    //One chunk updater per core
    //manages the cache for working with chunks
    //chunks themselves just record their current state
    class ChunkUpdater
    {
    private:
        enum ChunkState
        {
            ChunkState_Uninitialized = 0,
            ChunkState_InitiallyPacked = 1,
            ChunkState_UnpackedData = (1 << 1),
            ChunkState_UnpackedMasks = (1 << 2),
            ChunkState_UpdateDataPending = (1 << 3),
            ChunkState_UpdateMasksPending = (1 << 4),
            ChunkState_Unpacked = ChunkState_UnpackedMasks | ChunkState_UnpackedData,
            ChunkState_UpdatePending = ChunkState_UpdateDataPending | ChunkState_UpdateMasksPending,
        };

        uint64_t *mask_cache;
        uint8_t *decompressed_cache;
        uint8_t *compressed_cache;
        Chunk *active_chunk;
        ChunkUpdater::ChunkState active_state;

        void DecodeAll(bool mask_update_needed);

    public:
        ChunkUpdater();
        ~ChunkUpdater();

        void UnpackChunk(Chunk *chnk);

        //Mesh building - require decompression
        uint32_t GetCompiledLength();
        uint32_t Compile(uint32_t *inds_p);
        //Block updates - require decompression
        void SetBlock(uint8_t x, uint8_t y, uint8_t z, uint8_t val);
        void SetColumn(uint8_t x, uint8_t z, uint8_t val);
        void SetAll(uint8_t val);
        //Ray casts - only require masks
        void RayCast();
    };
} // namespace ProtoVoxel::Voxel