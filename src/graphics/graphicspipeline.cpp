#include "graphicspipeline.h"

#include <stdexcept>

#include "graphicsdevice.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::GraphicsPipeline::GraphicsPipeline() {
    this->depthTest = GL_EQUAL;

    ssbos = new BufferBinding[GraphicsDevice::MAX_BINDPOINTS];
    ubos = new BufferBinding[GraphicsDevice::MAX_BINDPOINTS];
    textures = new TextureBinding[GraphicsDevice::MAX_BINDPOINTS];
    for (int i = 0; i < GraphicsDevice::MAX_BINDPOINTS; i++) {
        ssbos[i].valid = false;
        ubos[i].valid = false;
        textures[i].valid = false;
    }
}

PVG::GraphicsPipeline::~GraphicsPipeline() {
    delete[] textures;
    delete[] ubos;
    delete[] ssbos;
}

void PVG::GraphicsPipeline::SetDepthTest(GLenum depthTest) {
    this->depthTest = depthTest;
}

void PVG::GraphicsPipeline::SetFramebuffer(std::weak_ptr<Framebuffer> fbuf) {
    this->fbuf = fbuf;
}

void PVG::GraphicsPipeline::SetClearColor(glm::vec4 const& vec) {
    this->clear_color = vec;
}

void PVG::GraphicsPipeline::SetDepth(float depth) {
    this->clear_depth = depth;
}

void PVG::GraphicsPipeline::SetIndirectBuffer(std::weak_ptr<GpuBuffer> indirect,
    size_t offset,
    size_t sz) {
    this->indirectBuffer.buffer = indirect;
    this->indirectBuffer.offset = offset;
    this->indirectBuffer.sz = sz;
    this->indirectBuffer.valid = !indirect.expired();
}

void PVG::GraphicsPipeline::SetIndexBuffer(std::weak_ptr<GpuBuffer> index,
    size_t offset,
    size_t sz) {
    this->indexBuffer.buffer = index;
    this->indexBuffer.offset = offset;
    this->indexBuffer.sz = sz;
    this->indexBuffer.valid = !index.expired();
}

void PVG::GraphicsPipeline::SetShaderProgram(
    std::weak_ptr<ShaderProgram> program) {
    this->program = program;
}

void PVG::GraphicsPipeline::SetSSBO(int bindpoint,
    std::weak_ptr<GpuBuffer> buffer,
    size_t offset,
    size_t sz) {
    if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
        throw std::invalid_argument("bindpoint out of range.");
    ssbos[bindpoint].buffer = buffer;
    ssbos[bindpoint].offset = offset;
    ssbos[bindpoint].sz = sz;
    ssbos[bindpoint].valid = !buffer.expired();
}

void PVG::GraphicsPipeline::SetUBO(int bindpoint,
    std::weak_ptr<GpuBuffer> buffer,
    size_t offset,
    size_t sz) {
    if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
        throw std::invalid_argument("bindpoint out of range.");
    ubos[bindpoint].buffer = buffer;
    ubos[bindpoint].offset = offset;
    ubos[bindpoint].sz = sz;
    ubos[bindpoint].valid = !buffer.expired();
}

void PVG::GraphicsPipeline::SetTexture(int bindpoint,
    std::weak_ptr<Texture> texture) {
    if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
        throw std::invalid_argument("bindpoint out of range.");
    textures[bindpoint].texture = texture;
    textures[bindpoint].valid = !texture.expired();
}