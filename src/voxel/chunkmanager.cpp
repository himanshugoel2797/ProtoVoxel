#include "chunkmanager.h"
#include "core/freecamera.h"
#include "graphics/graphicsdevice.h"
#include "voxel/PerlinNoise.h"
#include <chrono>
#include <iostream>
#include "core/frustumcull.h"

//for now just a fixed size grid
//chunkjobmanager class manages per thread resources for chunks like temporary memory to which chunks are decompressed
//only chunks in a small radius around an active object are kept uncompressed at any time
//

namespace PVV = ProtoVoxel::Voxel;
namespace PVG = ProtoVoxel::Graphics;

PVV::ChunkManager::ChunkManager()
{
}

PVV::ChunkManager::~ChunkManager() {}

void PVV::ChunkManager::Initialize(ChunkPalette& palette)
{
	this->palette = palette;
	pos_buf.SetStorage(4 * sizeof(uint32_t) * GridLen, GL_DYNAMIC_STORAGE_BIT);
    out_pos_buf.SetStorage(4 * sizeof(uint32_t) * GridLen, GL_DYNAMIC_STORAGE_BIT);
    out_splats_pos_buf.SetStorage(4 * sizeof(uint32_t) * GridLen, GL_DYNAMIC_STORAGE_BIT);
	out_occluded_pos_buf.SetStorage(4 * sizeof(uint32_t) * GridLen, GL_DYNAMIC_STORAGE_BIT);
    
	out_draw_buffer.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
    out_splats_buffer.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_occluded_draw_buf.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);

	mesh_mem.Initialize();
	uint32_t idx_offset = 0;
	uint32_t pos_offset = 0;

	siv::PerlinNoise noise(0);

	auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	draw_count = 0;
	draw_cmds.BeginFrame();
	for (int i = 0; i < GridLen; i++)
	{
		auto posvec = glm::ivec3((i % GridSide) * 30, (i / (GridSide * GridSide)) * 62, ((i / GridSide) % GridSide) * 30);
		positions[pos_offset] = glm::ivec4(posvec, 0);
		chnks[i].Initialize();
		chnks[i].SetPosition(posvec);

		chnk_updater.UnpackChunk(&chnks[i]);
		for (int x = -1; x < 31; x++)
			for (int z = -1; z < 31; z++)
				for (int y = -1; y < 63; y++)
				{
					auto d = noise.noise3D_0_1((posvec.x + x) * 0.005, (posvec.y + y) * 0.005, (posvec.z + z) * 0.005);
					if (d > 0.5)
						//auto d = noise.noise3D_0_1((posvec.x + x) * 0.005, 0 * 0.005, (posvec.z + z) * 0.005);
						//for (int y = posvec.y; y < d * 240 && y < posvec.y + 64; y++)
						chnk_updater.SetBlock(x + 1, y + 1, z + 1, 1);
				}

		uint32_t loopback_cntr = 0;
		auto count = chnk_updater.GetCompiledLength();
		auto mem_blk = mesh_mem.Alloc(count, &loopback_cntr);
		auto real_len = chnk_updater.Compile(mem_blk);
		mesh_mem.Flush(idx_offset, real_len, mem_blk);
		mesh_mem.FreeRear(count - real_len);

		if (count > 0)
		{
			struct draw_data_t dd;
			dd.start_idx = idx_offset;
			dd.len = count;
			dd.pos = posvec;
			
            draws.push_back(dd);
            draw_cmds.RecordDraw(count, 0, idx_offset, 0, 1);
            positions[draw_count] = glm::ivec4(posvec, 0);
			
            idx_offset += count;
			pos_offset++;
			draw_count++;
		}
	}
	draw_cmds.EndFrame();
	auto stopTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::cout << "Generation time: " << (stopTime - startTime) / 1000000.0 << "ms" << std::endl;
	std::cout << "Splatted Voxels: " << idx_offset << std::endl;
	pos_buf.Update(0, 16 * GridLen, positions);

	PVG::ShaderSource vert(GL_VERTEX_SHADER), frag(GL_FRAGMENT_SHADER);
    vert.SetSourceFile("shaders/splatting/vertex.glsl");
	vert.Compile();

    frag.SetSourceFile("shaders/splatting/fragment.glsl");
	frag.Compile();

	render_prog.Attach(vert);
	render_prog.Attach(frag);
	render_prog.Link();


	PVG::ShaderSource vert_(GL_VERTEX_SHADER), frag_(GL_FRAGMENT_SHADER);
	vert_.SetSourceFile("shaders/splatting/resolve_vtx.glsl");
	vert_.Compile();

	frag_.SetSourceFile("shaders/splatting/resolve_frag.glsl");
	frag_.Compile();

	resolve_prog.Attach(vert_);
	resolve_prog.Attach(frag_);
	resolve_prog.Link();

	PVG::ShaderSource bucket_shader(GL_COMPUTE_SHADER);
    bucket_shader.SetSourceFile("shaders/splatting/bucket.glsl");
	bucket_shader.Compile();
    bucket_prog.Attach(bucket_shader);
    bucket_prog.Link();

    PVG::ShaderSource splat_shader(GL_COMPUTE_SHADER);
    splat_shader.SetSourceFile("shaders/splatting/points.glsl");
    splat_shader.Compile();
    splat_prog.Attach(splat_shader);
    splat_prog.Link();

	PVG::ShaderSource occluded_bucket(GL_COMPUTE_SHADER);
	occluded_bucket.SetSourceFile("shaders/splatting/occluded_bucket.glsl");
	occluded_bucket.Compile();
	occludedCheck_prog.Attach(occluded_bucket);
	occludedCheck_prog.Link();

	fbuf = new PVG::Framebuffer(1024, 1024);

	colorTgt.SetStorage(GL_TEXTURE_2D, 1, GL_RGBA8, 1024, 1024);
	depthTgt.SetStorage(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 1024, 1024);
	prev_mip_pyramid.Initialize(&depthTgt, 1024, 1024, 11, true);
	cur_mip_pyramid.Initialize(&depthTgt, 1024, 1024, 11, false);

	GLenum fbuf_drawbufs[] = { GL_COLOR_ATTACHMENT0 };
	fbuf->Attach(GL_COLOR_ATTACHMENT0, colorTgt, 0);
	fbuf->Attach(GL_DEPTH_ATTACHMENT, depthTgt, 0);
	fbuf->DrawBuffers(1, fbuf_drawbufs);

	pointBuffer.SetStorage(GL_TEXTURE_2D, 1, GL_R32UI, 1024, 1024);

    bucketPipeline.SetIndirectBuffer(draw_cmds.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
    bucketPipeline.SetShaderProgram(&bucket_prog);
    bucketPipeline.SetSSBO(0, &pos_buf, 0, 4 * sizeof(float) * GridLen);
    bucketPipeline.SetSSBO(1, draw_cmds.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
    bucketPipeline.SetSSBO(2, &out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
    bucketPipeline.SetSSBO(3, &out_splats_buffer, 0, PVV::DrawCmdList::ListSize);
    bucketPipeline.SetSSBO(4, &out_pos_buf, 0, GridLen * 16);
    bucketPipeline.SetSSBO(5, &out_splats_pos_buf, 0, GridLen * 16);
	bucketPipeline.SetSSBO(6, &out_occluded_draw_buf, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(7, &out_occluded_pos_buf, 0, GridLen * 16);
	bucketPipeline.SetTexture(0, prev_mip_pyramid.GetTexture());

	occludedTestPipeline.SetIndirectBuffer(draw_cmds.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
	occludedTestPipeline.SetShaderProgram(&occludedCheck_prog);
	occludedTestPipeline.SetSSBO(0, &out_occluded_pos_buf, 0, GridLen * 16);
	occludedTestPipeline.SetSSBO(1, &out_occluded_draw_buf, 0, PVV::DrawCmdList::ListSize);
	occludedTestPipeline.SetSSBO(2, &out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	occludedTestPipeline.SetSSBO(3, &out_pos_buf, 0, GridLen * 16);
	occludedTestPipeline.SetTexture(0, cur_mip_pyramid.GetTexture());

	splatPipeline.SetIndirectBuffer(&out_splats_buffer, 0, PVV::DrawCmdList::ListSize);
	splatPipeline.SetShaderProgram(&splat_prog);
	splatPipeline.SetSSBO(0, &out_splats_pos_buf, 0, 4 * sizeof(float) * GridLen);
	splatPipeline.SetSSBO(1, &out_splats_buffer, 0, PVV::DrawCmdList::ListSize);
	splatPipeline.SetSSBO(2, mesh_mem.GetBuffer(), 0, 500 * 1024 * 1024);
	splatPipeline.SetImage(0, &pointBuffer, GL_R32UI, GL_READ_WRITE, 0);

	pipeline.SetDepth(0);
	pipeline.SetDepthTest(GL_GEQUAL);
	pipeline.SetShaderProgram(&render_prog);
	pipeline.SetFramebuffer(fbuf);
	pipeline.SetSSBO(1, &out_pos_buf, 0, 4 * sizeof(float) * GridLen);
	pipeline.SetSSBO(2, mesh_mem.GetBuffer(), 0, 4 * idx_offset);
	pipeline.SetUBO(2, palette.GetBuffer(), 0, 4 * sizeof(float) * 256);
	pipeline.SetIndirectBuffer(&out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	pipeline.SetIndexBuffer(mesh_mem.GetBuffer(), 0, PVV::DrawCmdList::ListSize);

	occludedPipeline.SetDepth(0);
	occludedPipeline.SetDepthTest(GL_GEQUAL);
	occludedPipeline.SetShaderProgram(&render_prog);
	occludedPipeline.SetFramebuffer(fbuf);
	occludedPipeline.SetSSBO(1, &out_pos_buf, 0, 4 * sizeof(float) * GridLen);
	occludedPipeline.SetSSBO(2, mesh_mem.GetBuffer(), 0, 4 * idx_offset);
	occludedPipeline.SetUBO(2, palette.GetBuffer(), 0, 4 * sizeof(float) * 256);
	occludedPipeline.SetIndirectBuffer(&out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	occludedPipeline.SetIndexBuffer(mesh_mem.GetBuffer(), 0, PVV::DrawCmdList::ListSize);

	resolvePipeline.SetDepth(0);
	resolvePipeline.SetDepthTest(GL_GEQUAL);
	resolvePipeline.SetShaderProgram(&resolve_prog);
	resolvePipeline.SetFramebuffer(fbuf);
	resolvePipeline.SetImage(0, &pointBuffer, GL_R32UI, GL_READ_WRITE, 0);
}

void PVV::ChunkManager::Update(glm::vec4 camPos, glm::mat4 vp, PVG::GpuBuffer* camera_buffer)
{
	std::sort(draws.begin(), draws.end(), [camPos](struct draw_data_t& a, struct draw_data_t& b) {
		auto diff0 = glm::vec3(camPos.x - a.pos.x + 15, camPos.y - a.pos.y + 31, camPos.z - a.pos.z + 15);
		auto diff1 = glm::vec3(camPos.x - b.pos.x + 15, camPos.y - b.pos.y + 31, camPos.z - b.pos.z + 15);

		return glm::dot(diff0, diff0) < glm::dot(diff1, diff1);
		});

	Frustum fstm(vp);

	pipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	occludedPipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
    bucketPipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	occludedTestPipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
}

void PVV::ChunkManager::Render(PVG::GpuBuffer* camera_buffer, double time)
{
	prev_mip_pyramid.BuildPyramid(camera_buffer);
	glClearTexImage(pointBuffer.GetID(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

    out_draw_buffer.Clear(0, 4);
    out_splats_buffer.Clear(0, 4);
	out_occluded_draw_buf.Clear(0, 4);
    PVG::GraphicsDevice::BindComputePipeline(bucketPipeline);
    glDispatchCompute((draw_count + 1023) / 1024, 1, 1);

	PVG::GraphicsDevice::BindComputePipeline(splatPipeline);
	glDispatchCompute(200, 1, 1);

	PVG::GraphicsDevice::BindGraphicsPipeline(pipeline);
	PVG::GraphicsDevice::ClearAll();
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_PROGRAM_POINT_SIZE);

	PVG::GraphicsDevice::MultiDrawIndirectCount(PVG::Topology::Points, 16, 0, 0, draw_count);

	//Test occluded chunks against the current frame to fix false negatives
	cur_mip_pyramid.BuildPyramid(camera_buffer);
	PVG::GraphicsDevice::BindComputePipeline(occludedTestPipeline);
	out_draw_buffer.Clear(0, 4);
	glDispatchCompute((draw_count + 1023) / 1024, 1, 1);

	PVG::GraphicsDevice::BindGraphicsPipeline(occludedPipeline);
	PVG::GraphicsDevice::MultiDrawIndirectCount(PVG::Topology::Points, 16, 0, 0, draw_count);

	PVG::GraphicsDevice::BindGraphicsPipeline(resolvePipeline);
	glDisable(GL_CULL_FACE);
	glDrawArrays(GL_TRIANGLES, 0, 3);


	glBlitNamedFramebuffer(fbuf->GetID(), 0, 0, 0, 1024, 1024, 0, 0, 1024, 1024, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
