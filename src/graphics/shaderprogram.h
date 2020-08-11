#pragma once
#include <stddef.h>
#include <stdint.h>
#include "glad/glad.h"


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
        uint32_t GetID() const {
            return id;
        }
    };
}  // namespace ProtoVoxel::Graphics