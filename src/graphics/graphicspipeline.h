#pragma once
#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "glad/glad.h"
#include "glm/vec4.hpp"

#include "framebuffer.h"
#include "gpubuffer.h"
#include "shaderprogram.h"
#include "texture.h"

namespace ProtoVoxel::Graphics
{
	class GraphicsDevice;
	class GraphicsPipeline
	{
	private:
		struct BufferBinding
		{
			bool valid;
			GpuBuffer* buffer;
			size_t offset;
			size_t sz;
		};

		struct TextureBinding
		{
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

		GLenum depthTest;
		Framebuffer* fbuf;
		ShaderProgram* program;
		struct BufferBinding indirectBuffer;
		struct BufferBinding indexBuffer;
		struct BufferBinding* ssbos;
		struct BufferBinding* ubos;
		struct TextureBinding* textures;
		struct ImageBinding* images;

		glm::vec4 clear_color;
		float clear_depth = 0;

		friend class GraphicsDevice;

	public:
		GraphicsPipeline();
		~GraphicsPipeline();

		void SetClearColor(glm::vec4 const& vec);
		void SetDepth(float clearDepth);

		void SetDepthTest(GLenum depthTest);
		void SetFramebuffer(Framebuffer* fbuf);
		void SetShaderProgram(ShaderProgram* program);
		void SetIndirectBuffer(GpuBuffer* indirect,
			size_t offset,
			size_t sz);
		void SetIndexBuffer(GpuBuffer* index, size_t offset, size_t sz);

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
} // namespace ProtoVoxel::Graphics