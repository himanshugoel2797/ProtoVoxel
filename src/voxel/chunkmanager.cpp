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
	mesh_mem.Initialize();
	jobManager.Initialize(&mesh_mem, &draw_cmds);

	this->palette = palette;
	out_draw_buffer.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_draw_buffer.SetName("Out Draw Calls");

	out_splats_buffer1.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_splats_buffer1.SetName("256 splats");

	out_splats_buffer2.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_splats_buffer2.SetName("2*256 splats");

	out_splats_buffer5.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_splats_buffer5.SetName("5*256 splats");

	out_splats_buffer10.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_splats_buffer10.SetName("10*256 splats");

	out_splats_bufferX.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_splats_bufferX.SetName("Variable size splats");

	out_occluded_draw_buf.SetStorage(PVV::DrawCmdList::ListSize, GL_DYNAMIC_STORAGE_BIT);
	out_occluded_draw_buf.SetName("Occluded Draw Calls");

	uint32_t idx_offset = 0;
	uint32_t pos_offset = 0;

	auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	draw_cmds.BeginFrame();
	for (int i = 0; i < GridLen; i++)
	{
		auto posvec = glm::ivec3((i % GridSide) * 30, (i / (GridSide * GridSide)) * 62, ((i / GridSide) % GridSide) * 30);
		positions[pos_offset] = glm::ivec4(posvec, 0);
		chnks[i].Initialize(i);
		chnks[i].SetPosition(posvec);

		auto tmp = glm::mat4(1);
		jobManager.RequestBuild(&chnks[i]);
		jobManager.RequestRemesh(&chnks[i]);
		jobManager.RequestCull(&chnks[i], tmp);
	}
	jobManager.EndFrame();

	auto stopTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::cout << "Generation time: " << (stopTime - startTime) / 1000000.0 << "ms" << std::endl;
	std::cout << "Splatted Voxels: " << idx_offset << std::endl;

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
	bucketPipeline.SetSSBO(0, draw_cmds.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(1, &out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(2, &out_splats_buffer1, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(3, &out_splats_buffer2, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(4, &out_splats_buffer5, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(5, &out_splats_buffer10, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(6, &out_splats_bufferX, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetSSBO(7, &out_occluded_draw_buf, 0, PVV::DrawCmdList::ListSize);
	bucketPipeline.SetTexture(0, prev_mip_pyramid.GetTexture());

	occludedTestPipeline.SetIndirectBuffer(draw_cmds.GetBuffer(), 0, PVV::DrawCmdList::ListSize);
	occludedTestPipeline.SetShaderProgram(&occludedCheck_prog);
	occludedTestPipeline.SetSSBO(0, &out_occluded_draw_buf, 0, PVV::DrawCmdList::ListSize);
	occludedTestPipeline.SetSSBO(1, &out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	occludedTestPipeline.SetTexture(0, cur_mip_pyramid.GetTexture());

	splatPipeline.SetIndirectBuffer(&out_splats_buffer1, 0, PVV::DrawCmdList::ListSize);
	splatPipeline.SetShaderProgram(&splat_prog);
	splatPipeline.SetSSBO(0, &out_splats_buffer1, 0, PVV::DrawCmdList::ListSize);
	splatPipeline.SetSSBO(1, mesh_mem.GetBuffer(), 0, 500 * 1024 * 1024);
	splatPipeline.SetImage(0, &pointBuffer, GL_R32UI, GL_READ_WRITE, 0);

	pipeline.SetDepth(0);
	pipeline.SetDepthTest(GL_GEQUAL);
	pipeline.SetShaderProgram(&render_prog);
	pipeline.SetFramebuffer(fbuf);
	pipeline.SetSSBO(0, &out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	pipeline.SetSSBO(1, mesh_mem.GetBuffer(), 0, PVV::MeshMalloc::MallocPoolSize);
	pipeline.SetUBO(1, palette.GetBuffer(), 0, 4 * sizeof(float) * 256);
	pipeline.SetIndirectBuffer(&out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	pipeline.SetIndexBuffer(mesh_mem.GetBuffer(), 0, PVV::DrawCmdList::ListSize);

	occludedPipeline.SetDepth(0);
	occludedPipeline.SetDepthTest(GL_GEQUAL);
	occludedPipeline.SetShaderProgram(&render_prog);
	occludedPipeline.SetFramebuffer(fbuf);
	occludedPipeline.SetSSBO(0, &out_draw_buffer, 0, PVV::DrawCmdList::ListSize);
	occludedPipeline.SetSSBO(1, mesh_mem.GetBuffer(), 0, PVV::MeshMalloc::MallocPoolSize);
	occludedPipeline.SetUBO(1, palette.GetBuffer(), 0, 4 * sizeof(float) * 256);
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
	for (int i = 0; i < GridLen; i++)
	{
		//jobManager.RequestCull(&chnks[i], vp);
	}

	pipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	occludedPipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	bucketPipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	occludedTestPipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	resolvePipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
}

void PVV::ChunkManager::Render(PVG::GpuBuffer* camera_buffer, double time)
{
	jobManager.FinishCurrentTasks();
	mesh_mem.Update();
	draw_count = draw_cmds.EndFrame();

	prev_mip_pyramid.BuildPyramid(camera_buffer);
	glClearTexImage(pointBuffer.GetID(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

	out_draw_buffer.Clear(0, 4);
	out_splats_bufferX.Clear(0, 4);
	out_splats_buffer1.Clear(0, 4);
	out_splats_buffer2.Clear(0, 4);
	out_splats_buffer5.Clear(0, 4);
	out_splats_buffer10.Clear(0, 4);
	out_occluded_draw_buf.Clear(0, 4);

	PVG::GraphicsDevice::BindComputePipeline(bucketPipeline);
	glDispatchCompute((draw_count + 63) / 64, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	{
		splatPipeline.SetIndirectBuffer(&out_splats_buffer1, 0, PVV::DrawCmdList::ListSize);
		splatPipeline.SetSSBO(0, &out_splats_buffer1, 0, PVV::DrawCmdList::ListSize);
		PVG::GraphicsDevice::BindComputePipeline(splatPipeline);
		glDispatchComputeIndirect(0);
	}
	{
		splatPipeline.SetIndirectBuffer(&out_splats_buffer2, 0, PVV::DrawCmdList::ListSize);
		splatPipeline.SetSSBO(0, &out_splats_buffer2, 0, PVV::DrawCmdList::ListSize);
		PVG::GraphicsDevice::BindComputePipeline(splatPipeline);
		glDispatchComputeIndirect(0);
	}
	{
		splatPipeline.SetIndirectBuffer(&out_splats_buffer5, 0, PVV::DrawCmdList::ListSize);
		splatPipeline.SetSSBO(0, &out_splats_buffer5, 0, PVV::DrawCmdList::ListSize);
		PVG::GraphicsDevice::BindComputePipeline(splatPipeline);
		glDispatchComputeIndirect(0);
	}
	{
		splatPipeline.SetIndirectBuffer(&out_splats_buffer10, 0, PVV::DrawCmdList::ListSize);
		splatPipeline.SetSSBO(0, &out_splats_buffer10, 0, PVV::DrawCmdList::ListSize);
		PVG::GraphicsDevice::BindComputePipeline(splatPipeline);
		glDispatchComputeIndirect(0);
	}
	{
		splatPipeline.SetIndirectBuffer(&out_splats_bufferX, 0, PVV::DrawCmdList::ListSize);
		splatPipeline.SetSSBO(0, &out_splats_bufferX, 0, PVV::DrawCmdList::ListSize);
		PVG::GraphicsDevice::BindComputePipeline(splatPipeline);
		glDispatchComputeIndirect(0);
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	PVG::GraphicsDevice::BindGraphicsPipeline(pipeline);
	PVG::GraphicsDevice::ClearAll();
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_PROGRAM_POINT_SIZE);

	PVG::GraphicsDevice::MultiDrawIndirectCount(PVG::Topology::Points, 16, 0, PVV::DrawCmdList::Stride, draw_count);

	PVG::GraphicsDevice::BindGraphicsPipeline(resolvePipeline);
	glDisable(GL_CULL_FACE);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Test occluded chunks against the current frame to fix false negatives
	cur_mip_pyramid.BuildPyramid(camera_buffer);
	PVG::GraphicsDevice::BindComputePipeline(occludedTestPipeline);
	out_draw_buffer.Clear(0, 4);
	glDispatchComputeIndirect(4);

	PVG::GraphicsDevice::BindGraphicsPipeline(occludedPipeline);
	PVG::GraphicsDevice::MultiDrawIndirectCount(PVG::Topology::Points, 16, 0, PVV::DrawCmdList::Stride, draw_count);



	glBlitNamedFramebuffer(fbuf->GetID(), 0, 0, 0, 1024, 1024, 0, 0, 1024, 1024, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//draw_cmds.BeginFrame();
	jobManager.EndFrame();
}
