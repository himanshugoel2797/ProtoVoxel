#include "graphicspipeline.h"

#include <stdexcept>

#include "graphicsdevice.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::GraphicsPipeline::GraphicsPipeline() : clear_color(0, 0, 0, 0)
{
    this->depthTest = GL_EQUAL;

    ssbos = new BufferBinding[GraphicsDevice::MAX_BINDPOINTS];
    ubos = new BufferBinding[GraphicsDevice::MAX_BINDPOINTS];
    textures = new TextureBinding[GraphicsDevice::MAX_BINDPOINTS];
    images = new ImageBinding[GraphicsDevice::MAX_BINDPOINTS];
    for (int i = 0; i < GraphicsDevice::MAX_BINDPOINTS; i++)
    {
        ssbos[i].valid = false;
        ubos[i].valid = false;
        textures[i].valid = false;
        images[i].valid = false;
    }

    indirectBuffer.valid = false;
    indexBuffer.valid = false;
}

PVG::GraphicsPipeline::~GraphicsPipeline()
{
    delete[] images;
    delete[] textures;
    delete[] ubos;
    delete[] ssbos;
}

void PVG::GraphicsPipeline::SetDepthTest(GLenum depthTest)
{
    this->depthTest = depthTest;
}

void PVG::GraphicsPipeline::SetFramebuffer(Framebuffer* fbuf)
{
    this->fbuf = fbuf;
}

void PVG::GraphicsPipeline::SetClearColor(glm::vec4 const &vec)
{
    this->clear_color = vec;
}

void PVG::GraphicsPipeline::SetDepth(float depth)
{
    this->clear_depth = depth;
}

void PVG::GraphicsPipeline::SetIndirectBuffer(GpuBuffer* indirect,
                                              size_t offset,
                                              size_t sz)
{
    this->indirectBuffer.buffer = indirect;
    this->indirectBuffer.offset = offset;
    this->indirectBuffer.sz = sz;
    this->indirectBuffer.valid = (indirect != nullptr);
}

void PVG::GraphicsPipeline::SetIndexBuffer(GpuBuffer* index,
                                           size_t offset,
                                           size_t sz)
{
    this->indexBuffer.buffer = index;
    this->indexBuffer.offset = offset;
    this->indexBuffer.sz = sz;
    this->indexBuffer.valid = (index != nullptr);
}

void PVG::GraphicsPipeline::SetShaderProgram(
    ShaderProgram* program)
{
    this->program = program;
}

void PVG::GraphicsPipeline::SetSSBO(int bindpoint,
                                    GpuBuffer* buffer,
                                    size_t offset,
                                    size_t sz)
{
    if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
        throw std::invalid_argument("bindpoint out of range.");
    ssbos[bindpoint].buffer = buffer;
    ssbos[bindpoint].offset = offset;
    ssbos[bindpoint].sz = sz;
    ssbos[bindpoint].valid = (buffer != nullptr);
}

void PVG::GraphicsPipeline::SetUBO(int bindpoint,
                                   GpuBuffer* buffer,
                                   size_t offset,
                                   size_t sz)
{
    if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
        throw std::invalid_argument("bindpoint out of range.");
    ubos[bindpoint].buffer = buffer;
    ubos[bindpoint].offset = offset;
    ubos[bindpoint].sz = sz;
    ubos[bindpoint].valid = (buffer != nullptr);
}

void PVG::GraphicsPipeline::SetTexture(int bindpoint,
                                       Texture* texture)
{
    if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
        throw std::invalid_argument("bindpoint out of range.");
    textures[bindpoint].texture = texture;
    textures[bindpoint].valid = (texture != nullptr);
}

void PVG::GraphicsPipeline::SetImage(int bindpoint,
    Texture* texture, GLenum format, GLenum rw, int lvl)
{
    if (bindpoint >= GraphicsDevice::MAX_BINDPOINTS)
        throw std::invalid_argument("bindpoint out of range.");
    images[bindpoint].texture = texture;
    images[bindpoint].format = format;
    images[bindpoint].rw = rw;
    images[bindpoint].lvl = lvl;
    images[bindpoint].valid = (texture != nullptr);
}