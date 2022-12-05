#pragma once
#include "glad/glad.h"
#include <stddef.h>
#include <stdint.h>
#include <memory>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cuda_gl_interop.h>

namespace ProtoVoxel::Graphics {
    class Texture {
    private:
        uint32_t id;
        GLenum target;
        cudaGraphicsResource_t cuda_res;
        int w, h, d;

    public:
        Texture();
        ~Texture();

        void SetStorage(GLenum target, int levels, int internalFormat, size_t w,
            size_t h, size_t d);
        void SetStorage(GLenum target, int levels, int internalFormat, size_t w,
            size_t h);
        void SetStorage(GLenum target, int levels, int internalFormat, size_t w);
        void Clear(int lv, int internalFormat, int type = GL_BYTE);

        cudaSurfaceObject_t GetCudaDevicePointer();
        void UnmapCudaDevicePointer(cudaSurfaceObject_t tex);

        inline void SetName(const char* name);
        
        GLenum GetTarget() const {
            return target;
        }
        uint32_t GetID() const {
            return id;
        }
        int GetWidth() const {
            return w;
        }
        int GetHeight() const {
            return h;
        }
        int GetDepth() const {
            return d;
        }

    };
} // namespace ProtoVoxel::Graphics

void ProtoVoxel::Graphics::Texture::SetName(const char* name) {
    glObjectLabel(GL_TEXTURE, id, strnlen_s(name, 16384), name);
}