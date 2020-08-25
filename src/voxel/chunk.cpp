#include "chunk.h"
#include "mortoncode.h"
#include <string.h>
#include <x86intrin.h>

namespace PVV = ProtoVoxel::Voxel;

PVV::Chunk::Chunk()
{
}

void PVV::Chunk::Initialize()
{
    set_voxel_cnt = 0;
    border_voxel_cnt = 0;
    memset(regional_voxel_cnt, 0, sizeof(regional_voxel_cnt));
    compressed_len = 0;
    compressed_data = nullptr;
}

void PVV::Chunk::SetPosition(glm::ivec3 &pos)
{
    position = pos;
}

glm::ivec3 &PVV::Chunk::GetPosition()
{
    return position;
}

PVV::Chunk::~Chunk()
{
}