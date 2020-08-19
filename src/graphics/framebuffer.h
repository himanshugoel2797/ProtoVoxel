#pragma once
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include "glad/glad.h"

#include "texture.h"

namespace ProtoVoxel::Core
{
    class Window;
}

namespace ProtoVoxel::Graphics
{
    class Framebuffer
    {
        friend class ProtoVoxel::Core::Window;

    private:
        uint32_t id;
        int w, h;

    public:
        static std::shared_ptr<Framebuffer> GetDefault();

        Framebuffer(uint32_t id, int w, int h);
        Framebuffer(int w, int h);
        ~Framebuffer();

        void Attach(GLenum attachment, Texture const &tex, int level);
        void Attach(GLenum attachment, Texture const &tex, int level, int layer);
        void DrawBuffers(int n, const GLenum *attachments);

        uint32_t GetID() const
        {
            return id;
        }
    };
} // namespace ProtoVoxel::Graphics