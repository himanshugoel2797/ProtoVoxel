#include "framebuffer.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::Framebuffer::Framebuffer(uint32_t id, int w, int h) {
  this->w = w;
  this->h = h;
  this->id = id;
}

PVG::Framebuffer::Framebuffer(int w, int h) {
  this->w = w;
  this->h = h;
  glGenFramebuffers(1, &id);
}

PVG::Framebuffer::~Framebuffer() { glDeleteFramebuffers(1, &id); }

void PVG::Framebuffer::Attach(GLenum attachment, PVG::Texture const &tex,
                              int level) {
  glNamedFramebufferTexture(id, attachment, tex.GetID(), level);
}

void PVG::Framebuffer::Attach(GLenum attachment, PVG::Texture const &tex,
                              int level, int layer) {
  glNamedFramebufferTextureLayer(id, attachment, tex.GetID(), level, layer);
}

void PVG::Framebuffer::DrawBuffers(int n, const GLenum *attachments) {
  glNamedFramebufferDrawBuffers(id, n, attachments);
}