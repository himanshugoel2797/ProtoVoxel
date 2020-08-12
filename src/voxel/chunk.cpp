#include "chunk.h"
#include "mortoncode.h"
#include <string.h>

namespace PVV = ProtoVoxel::Voxel;

PVV::Chunk::Chunk() {
    voxels = nullptr;
    codingScheme = ChunkCodingScheme::None;
}

void PVV::Chunk::SetAll(uint8_t val) {
    codingScheme = ChunkCodingScheme::SingleFull;
    allVal = val;
    if (voxels != nullptr) {
        delete[] voxels;
        voxels = nullptr;
    }
}

void PVV::Chunk::SetSingle(uint8_t x, uint8_t y, uint8_t z, uint8_t val) {
    uint32_t idx = PVV::MortonCode::Encode(x, y, z);

    if (codingScheme == ChunkCodingScheme::None)
        voxels[idx] = val;
}

PVV::Chunk::~Chunk() {

}