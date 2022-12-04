#include "chunkupdater.h"
#include "misc/intersections.h"
#include <immintrin.h>
#include <string.h>

namespace PPC = ProtoVoxel::PointCloud;
namespace PVM = ProtoVoxel::Misc;

PPC::ChunkUpdater::ChunkUpdater() { active_chunk = nullptr; }

PPC::ChunkUpdater::~ChunkUpdater() {}

void PPC::ChunkUpdater::UnpackChunk(PPC::Chunk *chnk) {
}

void PPC::ChunkUpdater::DecodeAll(bool mask_update_needed) {
}

uint32_t PPC::ChunkUpdater::GetCompiledLength() {
  return 0;//Returns point count
}

uint32_t PPC::ChunkUpdater::Compile(uint32_t *inds_p, glm::ivec3 &min_bounds,
                                    glm::ivec3 &max_bounds) {
  return 0;//Returns point count
}

void PPC::ChunkUpdater::SetBlock(uint8_t x, uint8_t y, uint8_t z, uint8_t val) {
}

void PPC::ChunkUpdater::SetColumn(uint8_t x, uint8_t z, uint8_t val) {
}

void PPC::ChunkUpdater::SetAll(uint8_t val) {
}

bool PPC::ChunkUpdater::RayCast(const glm::vec3 &o, const glm::vec3 &dir,
                                glm::vec3 &intersection) {
  return false;
}