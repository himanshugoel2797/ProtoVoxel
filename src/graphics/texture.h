#pragma once
#include "glad/glad.h"
#include <stddef.h>
#include <stdint.h>
#include <memory>

namespace ProtoVoxel::Graphics {
    class Texture {
    private:
        uint32_t id;
        GLenum target;

    public:
        Texture();
        ~Texture();

        void SetStorage(GLenum target, int levels, int internalFormat, size_t w,
            size_t h, size_t d);
        void SetStorage(GLenum target, int levels, int internalFormat, size_t w,
            size_t h);
        void SetStorage(GLenum target, int levels, int internalFormat, size_t w);
        void Clear(int lv, int internalFormat);

        inline void SetName(const char* name);
        
        GLenum GetTarget() const {
            return target;
        }
        uint32_t GetID() const {
            return id;
        }
    };
} // namespace ProtoVoxel::Graphics

void ProtoVoxel::Graphics::Texture::SetName(const char* name) {
    glObjectLabel(GL_TEXTURE, id, strnlen_s(name, 16384), name);
}