#include "texture.h"

namespace PVG = ProtoVoxel::Graphics;

PVG::Texture::Texture()
{
    id = -1;
}

PVG::Texture::~Texture()
{
    if (id != -1){
        glDeleteTextures(1, &id);
        cudaGraphicsUnregisterResource(cuda_res);
    }
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
    this->w = w;
    this->h = h;
    this->d = d;
    auto err = cudaGraphicsGLRegisterImage(&cuda_res, id, target, cudaGraphicsRegisterFlagsSurfaceLoadStore);
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

void ProtoVoxel::Graphics::Texture::Clear(int lv, int internalFormat, int type)
{
    glClearTexImage(id, lv, internalFormat, type, nullptr);
}

cudaSurfaceObject_t PVG::Texture::GetCudaDevicePointer()
{
    auto err0 = cudaGraphicsMapResources(1, &cuda_res, 0);

    cudaArray_t arr;
    auto err2 = cudaGraphicsSubResourceGetMappedArray(&arr, cuda_res, 0, 0);//cudaGetMipmappedArrayLevel(&arr, ptr, 0);

    cudaResourceDesc resDesc;
    memset(&resDesc, 0, sizeof(resDesc));
    resDesc.resType = cudaResourceTypeArray;
    resDesc.res.array.array = arr;

    cudaSurfaceObject_t surf;
    cudaCreateSurfaceObject(&surf, &resDesc);

    return surf;
}

void PVG::Texture::UnmapCudaDevicePointer(cudaSurfaceObject_t tex)
{
    cudaDestroySurfaceObject(tex);
    cudaGraphicsUnmapResources(1, &cuda_res, 0);
}