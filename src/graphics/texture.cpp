#include "texture.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::Texture::Texture()
{
    id = -1;
}

PVG::Texture::~Texture()
{
    if (id != -1)
        glDeleteTextures(1, &id);
}

void PVG::Texture::SetStorage(GLenum target, int levels, int internalFormat,
                              size_t w, size_t h, size_t d)
{
    if (id == -1)
        glCreateTextures(target, 1, &id);

    switch (target)
    {
    case GL_TEXTURE_1D:
        glTextureStorage1D(id, levels, internalFormat, w);
        break;
    case GL_TEXTURE_1D_ARRAY:
    case GL_TEXTURE_2D:
        glTextureStorage2D(id, levels, internalFormat, w, h);
        break;
    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_3D:
        glTextureStorage3D(id, levels, internalFormat, w, h, d);
        break;
    }

    this->target = target;
}

void PVG::Texture::SetStorage(GLenum target, int levels, int internalFormat,
                              size_t w, size_t h)
{
    SetStorage(target, levels, internalFormat, w, h, 1);
}

void PVG::Texture::SetStorage(GLenum target, int levels, int internalFormat,
                              size_t w)
{
    SetStorage(target, levels, internalFormat, w, 1, 1);
}