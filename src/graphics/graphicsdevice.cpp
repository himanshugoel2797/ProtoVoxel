#include "graphicsdevice.h"
#include "glad/glad.h"

#include <stdexcept>

namespace PVG = ProtoVoxel::Graphics;

static PVG::Framebuffer* boundFramebuffer;
static PVG::ShaderProgram* boundProgram;

static PVG::GpuBuffer* boundIndirectBuffer;
static PVG::GpuBuffer* boundIndexBuffer;
static PVG::GpuBuffer*
boundSSBOs[PVG::GraphicsDevice::MAX_BINDPOINTS];
static PVG::GpuBuffer*
boundUBOs[PVG::GraphicsDevice::MAX_BINDPOINTS];
static PVG::Texture*
boundTextures[PVG::GraphicsDevice::MAX_BINDPOINTS];
static PVG::Texture*
boundImages[PVG::GraphicsDevice::MAX_BINDPOINTS];

void PVG::GraphicsDevice::ClearColor()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void PVG::GraphicsDevice::ClearDepth()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void PVG::GraphicsDevice::ClearAll()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PVG::GraphicsDevice::BindGraphicsPipeline(
	PVG::GraphicsPipeline const& pipeline)
{
	if (pipeline.indirectBuffer.valid)
	{
		{
			boundIndirectBuffer = pipeline.indirectBuffer.buffer;
			auto indir_ptr = boundIndirectBuffer;
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indir_ptr->GetID());
			glBindBuffer(GL_PARAMETER_BUFFER, indir_ptr->GetID());
		}
	}
	else
	{
		boundIndirectBuffer = nullptr;
	}

	if (pipeline.indexBuffer.valid)
	{
		{
			boundIndexBuffer = pipeline.indexBuffer.buffer;
			auto index_ptr = boundIndexBuffer;
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_ptr->GetID());
		}
	}
	else
	{
		boundIndexBuffer = nullptr;
	}

	for (int i = 0; i < MAX_BINDPOINTS; i++)
	{
		if (pipeline.ssbos[i].valid)
		{
			{
				boundSSBOs[i] = pipeline.ssbos[i].buffer;
				auto ssbo_ptr = boundSSBOs[i];
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, ssbo_ptr->GetID(),
					pipeline.ssbos[i].offset, pipeline.ssbos[i].sz);
			}
		}
		else
			boundSSBOs[i] = nullptr;

		if (pipeline.ubos[i].valid)
		{
			{
				boundUBOs[i] = pipeline.ubos[i].buffer;
				auto ubo_ptr = boundUBOs[i];
				glBindBufferRange(GL_UNIFORM_BUFFER, i, ubo_ptr->GetID(),
					pipeline.ubos[i].offset, pipeline.ubos[i].sz);
			}
		}
		else
			boundUBOs[i] = nullptr;

		if (pipeline.images[i].valid)
		{
			{
				boundImages[i] = pipeline.images[i].texture;
				auto img_ptr = boundImages[i];
				glBindImageTexture(i, img_ptr->GetID(), pipeline.images[i].lvl, GL_FALSE, 0, pipeline.images[i].rw, pipeline.images[i].format);
			}
		}
		else
			boundImages[i] = nullptr;

		if (pipeline.textures[i].valid)
		{
			{
				boundTextures[i] = pipeline.textures[i].texture;
				auto img_ptr = boundTextures[i];
				glBindTextureUnit(i, img_ptr->GetID());
			}
		}
		else
			boundTextures[i] = nullptr;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, pipeline.fbuf->GetID());
	glUseProgram(pipeline.program->GetID());

	glClearColor(pipeline.clear_color.r, pipeline.clear_color.g,
		pipeline.clear_color.b, pipeline.clear_color.a);

	glClearDepthf(pipeline.clear_depth);
	glDepthFunc(pipeline.depthTest);
}

void PVG::GraphicsDevice::BindComputePipeline(
	PVG::ComputePipeline const& pipeline)
{
	if (pipeline.indirectBuffer.valid)
	{
		{
			boundIndirectBuffer = pipeline.indirectBuffer.buffer;
			auto indir_ptr = boundIndirectBuffer;
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indir_ptr->GetID());
			glBindBuffer(GL_PARAMETER_BUFFER, indir_ptr->GetID());
		}
	}
	else
	{
		boundIndirectBuffer = (nullptr);
	}

	for (int i = 0; i < MAX_BINDPOINTS; i++)
	{
		if (pipeline.ssbos[i].valid)
		{
			{
				boundSSBOs[i] = pipeline.ssbos[i].buffer;
				auto ssbo_ptr = boundSSBOs[i];
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, ssbo_ptr->GetID(),
					pipeline.ssbos[i].offset, pipeline.ssbos[i].sz);
			}
		}
		else
			boundSSBOs[i] = (nullptr);

		if (pipeline.ubos[i].valid)
		{
			{
				boundUBOs[i] = pipeline.ubos[i].buffer;
				auto ubo_ptr = boundUBOs[i];
				glBindBufferRange(GL_UNIFORM_BUFFER, i, ubo_ptr->GetID(),
					pipeline.ubos[i].offset, pipeline.ubos[i].sz);
			}
		}
		else
			boundUBOs[i] = (nullptr);

		if (pipeline.images[i].valid)
		{
			{
				boundImages[i] = pipeline.images[i].texture;
				auto img_ptr = boundImages[i];
				glBindImageTexture(i, img_ptr->GetID(), pipeline.images[i].lvl, GL_FALSE, 0, pipeline.images[i].rw, pipeline.images[i].format);
			}
		}
		else
			boundImages[i] = (nullptr);

		if (pipeline.textures[i].valid)
		{
			{
				boundTextures[i] = pipeline.textures[i].texture;
				auto img_ptr = boundTextures[i];
				glBindTextureUnit(i, img_ptr->GetID());
			}
		}
		else
			boundTextures[i] = (nullptr);
	}

	glUseProgram(pipeline.program->GetID());
}

void PVG::GraphicsDevice::MultiDrawElementsIndirectCount(Topology topo, IndexType type, int cmdOffset, int drawOffset, int stride, int maxDrawCount)
{
	int mode = 0;
	switch (topo)
	{
	case Topology::Triangles:
		mode = GL_TRIANGLES;
		break;
	case Topology::TriangleStrip:
		mode = GL_TRIANGLE_STRIP;
		break;
	case Topology::Points:
		mode = GL_POINTS;
		break;
	case Topology::Lines:
		mode = GL_LINES;
		break;
	case Topology::LineStrip:
		mode = GL_LINE_STRIP;
		break;
	}

	int idx_type = 0;
	switch (type)
	{
	case IndexType::UByte:
		idx_type = GL_UNSIGNED_BYTE;
		break;
	case IndexType::UShort:
		idx_type = GL_UNSIGNED_SHORT;
		break;
	case IndexType::UInt:
		idx_type = GL_UNSIGNED_INT;
		break;
	}

	glMultiDrawElementsIndirectCount(mode, idx_type, (void*)cmdOffset, drawOffset, maxDrawCount, stride);
}

void PVG::GraphicsDevice::MultiDrawElementsIndirect(Topology topo, IndexType type, int cmdOffset, int stride, int maxDrawCount)
{
	int mode = 0;
	switch (topo)
	{
	case Topology::Triangles:
		mode = GL_TRIANGLES;
		break;
	case Topology::TriangleStrip:
		mode = GL_TRIANGLE_STRIP;
		break;
	case Topology::Points:
		mode = GL_POINTS;
		break;
	case Topology::Lines:
		mode = GL_LINES;
		break;
	case Topology::LineStrip:
		mode = GL_LINE_STRIP;
		break;
	}

	int idx_type = 0;
	switch (type)
	{
	case IndexType::UByte:
		idx_type = GL_UNSIGNED_BYTE;
		break;
	case IndexType::UShort:
		idx_type = GL_UNSIGNED_SHORT;
		break;
	case IndexType::UInt:
		idx_type = GL_UNSIGNED_INT;
		break;
	}

	glMultiDrawElementsIndirect(mode, idx_type, (void*)cmdOffset, maxDrawCount, stride);
}

void PVG::GraphicsDevice::MultiDrawIndirect(Topology topo, int cmdOffset, int stride, int draw_count)
{
	int mode = 0;
	switch (topo)
	{
	case Topology::Triangles:
		mode = GL_TRIANGLES;
		break;
	case Topology::TriangleStrip:
		mode = GL_TRIANGLE_STRIP;
		break;
	case Topology::Points:
		mode = GL_POINTS;
		break;
	case Topology::Lines:
		mode = GL_LINES;
		break;
	case Topology::LineStrip:
		mode = GL_LINE_STRIP;
		break;
	}

	glMultiDrawArraysIndirect(mode, (void*)cmdOffset, draw_count, stride);
}

void PVG::GraphicsDevice::MultiDrawIndirectCount(Topology topo, int cmdOffset, int count_offset, int stride, int draw_count)
{
	int mode = 0;
	switch (topo)
	{
	case Topology::Triangles:
		mode = GL_TRIANGLES;
		break;
	case Topology::TriangleStrip:
		mode = GL_TRIANGLE_STRIP;
		break;
	case Topology::Points:
		mode = GL_POINTS;
		break;
	case Topology::Lines:
		mode = GL_LINES;
		break;
	case Topology::LineStrip:
		mode = GL_LINE_STRIP;
		break;
	}

	glMultiDrawArraysIndirectCount(mode, (void*)cmdOffset, count_offset, draw_count, stride);
}

void PVG::GraphicsDevice::DispatchCompute(uint32_t x, uint32_t y, uint32_t z) {
	glDispatchCompute(x, y, z);
}