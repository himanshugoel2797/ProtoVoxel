#pragma once
#include <stddef.h>
#include <stdint.h>
#include "glad/glad.h"
#include <memory>
#include "shadersource.h"

namespace ProtoVoxel::Graphics {
    class ShaderProgram {
    private:
        uint32_t id;

    public:
        ShaderProgram();
        ~ShaderProgram();
        void Attach(ShaderSource const& src);
        void Link();

        inline void SetName(const char* name);

        uint32_t GetID() const {
            return id;
        }
    };
}  // namespace ProtoVoxel::Graphics

void ProtoVoxel::Graphics::ShaderProgram::SetName(const char* name) {
    glObjectLabel(GL_PROGRAM, id, strnlen_s(name, 16384), name);
}