#include "chunk.h"
#include <string.h>
#include <immintrin.h>

namespace PPC = ProtoVoxel::PointCloud;

PPC::Chunk::Chunk()
{
    compressed_len = 0;
    compressed_data = nullptr;
}

void PPC::Chunk::Initialize(uint32_t id)
{
    this->id = id;
    compressed_len = 0;
    compressed_data = nullptr;
}

void PPC::Chunk::SetPosition(glm::ivec3 &pos)
{
    position = pos;
}

glm::ivec3 &PPC::Chunk::GetPosition()
{
    return position;
}

PPC::Chunk::~Chunk()
{
}