#pragma once
#include "glad/glad.h"
#include <stddef.h>
#include <stdint.h>
#include <memory>

namespace ProtoVoxel::Graphics
{
    class ShaderSource
    {
    private:
        GLenum shaderType;
        uint32_t id;

    public:
        ShaderSource(GLenum shaderType);
        ~ShaderSource();
        void SetSource(const char *src);
        void SetSourceFile(const char *src);
        void Compile();

        inline void SetName(const char* name);

        uint32_t GetID() const
        {
            return id;
        }
    };
} // namespace ProtoVoxel::Graphics

void ProtoVoxel::Graphics::ShaderSource::SetName(const char* name) {
    glObjectLabel(GL_SHADER, id, strnlen_s(name, 16384), name);
}