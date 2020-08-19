#include "framebuffer.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::Framebuffer::Framebuffer(uint32_t id, int w, int h)
{
    this->w = w;
    this->h = h;
    this->id = id;
}

PVG::Framebuffer::Framebuffer(int w, int h)
{
    this->w = w;
    this->h = h;
    glCreateFramebuffers(1, &id);
}

PVG::Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &id);
}

void PVG::Framebuffer::Attach(GLenum attachment,
                              PVG::Texture const &tex,
                              int level)
{
    glNamedFramebufferTexture(id, attachment, tex.GetID(), level);
}

void PVG::Framebuffer::Attach(GLenum attachment,
                              PVG::Texture const &tex,
                              int level,
                              int layer)
{
    glNamedFramebufferTextureLayer(id, attachment, tex.GetID(), level, layer);
}

void PVG::Framebuffer::DrawBuffers(int n, const GLenum *attachments)
{
    glNamedFramebufferDrawBuffers(id, n, attachments);
}

static std::shared_ptr<PVG::Framebuffer> default_fbuf;

std::shared_ptr<PVG::Framebuffer> PVG::Framebuffer::GetDefault()
{
    if (default_fbuf == nullptr)
    {
        default_fbuf = std::make_shared<PVG::Framebuffer>(0, 0, 0);
    }
    return default_fbuf;
}