#include "chunkpalette.h"

namespace PVV = ProtoVoxel::Voxel;
namespace PVG = ProtoVoxel::Graphics;

PVV::ChunkPalette::ChunkPalette()
{
}

PVV::ChunkPalette::~ChunkPalette()
{
}

void PVV::ChunkPalette::Initialize()
{
    palette.reserve(256);
    paletteBuffer.SetStorage(4 * sizeof(float) * 256, GL_DYNAMIC_STORAGE_BIT);
}

int PVV::ChunkPalette::Count()
{
    return palette.size();
}

uint8_t PVV::ChunkPalette::Register(glm::vec4 val)
{
    palette.push_back(val);
    paletteBuffer.Update(0, 4 * sizeof(float) * 256, palette.data());
    return palette.size() - 1;
}

PVG::GpuBuffer* PVV::ChunkPalette::GetBuffer()
{
    return &paletteBuffer;
}