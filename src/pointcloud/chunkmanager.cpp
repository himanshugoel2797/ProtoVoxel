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

namespace PPC = ProtoVoxel::PointCloud;
namespace PVG = ProtoVoxel::Graphics;

PPC::ChunkManager::ChunkManager()
{
}

PPC::ChunkManager::~ChunkManager() {}

typedef struct {
    int32_t pos[3];
    int32_t r : 8;
    int32_t g : 8;
    int32_t b : 8;
    int32_t a : 8;
} pointdata_t;
uint32_t pointsCount;
pointdata_t* points_data;

void PPC::ChunkManager::Initialize()
{
	mesh_mem.Initialize();
    int sideSteps = 256;
    float sideLen = 1.0f;
    pointsCount = sideSteps * sideSteps * 6;
    uint32_t loopback_cnt = 0;

	cudaMallocManaged(&points_data, pointsCount * sizeof(pointdata_t));
	pointdata_t* mem = points_data;//(pointdata_t*)mesh_mem.Alloc(pointsCount * sizeof(pointdata_t), loopback_cnt);
    int idx = 0;
    for (int face = 0; face < 3; face++)
    {
        for (int sign = 0; sign <= 1; sign += 1)
        {
            for (int y = 0; y < sideSteps; y++)
            for (int x = 0; x < sideSteps; x++)
            {
                float pos[3] = { 0, 0, 0 };
                pos[face] = sign * sideLen;
                pos[(face + 1) % 3] = (x / (float)(sideSteps - 1)) * sideLen;
                pos[(face + 2) % 3] = (y / (float)(sideSteps - 1)) * sideLen;

                //Normalize pos
				float len = sqrtf(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
                pos[0] /= len;
                pos[1] /= len;
                pos[2] /= len;

				mem[idx].r = (int)(pos[0] * 255);
				mem[idx].g = (int)(pos[1] * 255);
				mem[idx].b = (int)(pos[2] * 255);
				mem[idx].a = 255;

                mem[idx].pos[0] = (int)(pos[0] * POS_SCALE_FACTOR);
                mem[idx].pos[1] = (int)(pos[1] * POS_SCALE_FACTOR);
                mem[idx].pos[2] = (int)(pos[2] * POS_SCALE_FACTOR);
                idx++;
            }
        }
    }
    //mesh_mem.Flush((uint32_t*)mem, pointsCount * sizeof(pointdata_t));

	jobManager.Initialize(&mesh_mem, &draw_cmds);
	/*
	uint32_t idx_offset = 0;
	uint32_t pos_offset = 0;

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
	jobManager.EndFrame();*/

	PVG::ShaderSource splat_shader(GL_COMPUTE_SHADER);
	splat_shader.SetSourceFile("shaders/pointcloud/splat.glsl");
	splat_shader.Compile();
	splat_prog.Attach(splat_shader);
	splat_prog.Link();

	fbuf = new PVG::Framebuffer(1024, 1024);
	colorTgt.SetStorage(GL_TEXTURE_2D, 1, GL_RGBA8, 1024, 1024);
	pointBuffer.SetStorage(GL_TEXTURE_2D, 1, GL_R32UI, 1024, 1024);
	GLenum fbuf_drawbufs[] = { GL_COLOR_ATTACHMENT0 };
	fbuf->Attach(GL_COLOR_ATTACHMENT0, colorTgt, 0);
	fbuf->DrawBuffers(1, fbuf_drawbufs);

	fbuf0 = new PVG::Framebuffer(1024, 1024);
	colorTgt0.SetStorage(GL_TEXTURE_2D, 1, GL_RGBA8, 1024, 1024);
	fbuf0->Attach(GL_COLOR_ATTACHMENT0, colorTgt0, 0);
	fbuf0->DrawBuffers(1, fbuf_drawbufs);

	splatPipeline.SetIndirectBuffer(&splat_cmdbuffer, 0, PPC::DrawCmdList::ListSize);
	splatPipeline.SetShaderProgram(&splat_prog);
	splatPipeline.SetSSBO(0, mesh_mem.GetBuffer(), 0, 500 * 1024 * 1024);
	splatPipeline.SetImage(0, &pointBuffer, GL_R32UI, GL_READ_WRITE, 0);
    splatPipeline.SetImage(1, &colorTgt, GL_RGBA8, GL_READ_WRITE, 0);

	colorTgt_cur = &colorTgt;
	colorTgt_n = &colorTgt0;

	fbuf_cur = fbuf;
	fbuf_n = fbuf0;
}

void PPC::ChunkManager::Update(glm::vec4 camPos, glm::mat4 vp, PVG::GpuBuffer* camera_buffer)
{
	for (int i = 0; i < GridLen; i++)
	{
		//jobManager.RequestCull(&chnks[i], vp);
	}

    splatPipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	resolvePipeline.SetUBO(0, camera_buffer, 0, sizeof(ProtoVoxel::Core::GlobalParameters));
	auto tmp = colorTgt_cur;
	colorTgt_cur = colorTgt_n;
	colorTgt_n = tmp;

	auto tmp0 = fbuf_cur;
	fbuf_cur = fbuf_n;
	fbuf_n = tmp0;
}

void PPC::ChunkManager::Render(PVG::GpuBuffer* camera_buffer, double time)
{
	jobManager.FinishCurrentTasks();
	mesh_mem.Update();
	//draw_count = draw_cmds.EndFrame();

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    pointBuffer.Clear(0, GL_RED_INTEGER, GL_UNSIGNED_INT);
	colorTgt_cur->Clear(0, GL_RGBA);

	cudaSurfaceObject_t colorPtr = colorTgt_cur->GetCudaDevicePointer();
	void* srcPointPtr = points_data;//mesh_mem.GetBuffer()->GetCudaDevicePointer();
	void* global_vars = camera_buffer->GetCudaDevicePointer();
	void* pointBfr;
	cudaMallocAsync(&pointBfr, 1024 * 1024 * sizeof(uint32_t), 0);
	cudaMemsetAsync(pointBfr, 0, 1024 * 1024 * 4);

	//Call cuda-based point renderer
	splat(global_vars, srcPointPtr, pointBfr, colorPtr, 1024, 1024, pointsCount);

	cudaFreeAsync(pointBfr, 0);
	camera_buffer->UnmapCudaDevicePointer();
	//mesh_mem.GetBuffer()->UnmapCudaDevicePointer();
	colorTgt_cur->UnmapCudaDevicePointer(colorPtr);
	//PVG::GraphicsDevice::BindComputePipeline(splatPipeline);
    //glDispatchCompute(pointsCount / 128, 1, 1);

    //Show the point buffer on screen
	glBlitNamedFramebuffer(fbuf_cur->GetID(), 0, 0, 0, 1024, 1024, 0, 0, 1024, 1024, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//draw_cmds.BeginFrame();
	jobManager.EndFrame();
}
