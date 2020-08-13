#include "chunkpalette.h"

namespace PVV = ProtoVoxel::Voxel;

PVV::ChunkPalette::ChunkPalette()
{
}

PVV::ChunkPalette::~ChunkPalette()
{
}

int PVV::ChunkPalette::Count()
{
    return refCounts.size();
}

int PVV::ChunkPalette::Register(uint16_t val)
{
}