#pragma once
#include "glad/glad.h"
#include <stddef.h>
#include <stdint.h>

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
        uint32_t GetID() const
        {
            return id;
        }
    };
} // namespace ProtoVoxel::Graphics