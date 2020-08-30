#pragma once

#include <memory>

#include "gpubuffer.h"
#include "shaderprogram.h"
#include "texture.h"

namespace ProtoVoxel::Graphics {
    class GraphicsDevice;
    class ComputePipeline {
    private:
        struct BufferBinding {
            bool valid;
            GpuBuffer* buffer;
            size_t offset;
            size_t sz;
        };

        struct TextureBinding {
            bool valid;
            Texture* texture;
        };

        struct ImageBinding
        {
            bool valid;
            Texture* texture;
            GLint format;
            GLint rw;
            int lvl;
        };

        ShaderProgram* program;
        struct BufferBinding indirectBuffer;
        struct BufferBinding* ssbos;
        struct BufferBinding* ubos;
        struct TextureBinding* textures;
        struct ImageBinding* images;

        friend class GraphicsDevice;

    public:
        ComputePipeline();
        ~ComputePipeline();

        void SetShaderProgram(ShaderProgram* program);
        void SetIndirectBuffer(GpuBuffer* indirect,
            size_t offset,
            size_t sz);

        void SetSSBO(int bindpoint,
            GpuBuffer* buffer,
            size_t offset,
            size_t sz);
        void SetUBO(int bindpoint,
            GpuBuffer* buffer,
            size_t offset,
            size_t sz);
        void SetTexture(int bindpoint, Texture* texture);
        void SetImage(int bindpoint, Texture* texture, GLenum format, GLenum rw, int lvl);
    };
}  // namespace ProtoVoxel::Graphics