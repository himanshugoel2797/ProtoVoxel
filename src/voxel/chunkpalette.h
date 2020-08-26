#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include "graphics/gpubuffer.h"
#include "glm/glm.hpp"

namespace ProtoVoxel::Voxel
{
    class ChunkPalette
    {
    private:
        std::vector<glm::vec4> palette;
        std::shared_ptr<ProtoVoxel::Graphics::GpuBuffer> paletteBuffer;

    public:
        ChunkPalette();
        ~ChunkPalette();

        void Initialize();
        int Count();
        uint8_t Register(glm::vec4 val);
        std::weak_ptr<ProtoVoxel::Graphics::GpuBuffer> GetBuffer();
    };
} // namespace ProtoVoxel::Voxel