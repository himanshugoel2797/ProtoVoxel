#include "hiz.h"
#include "graphicsdevice.h"
#include "core/freecamera.h"

namespace PVG = ProtoVoxel::Graphics;
namespace PVC = ProtoVoxel::Core;

PVG::HiZ::HiZ()
{
}

PVG::HiZ::~HiZ()
{
}

void PVG::HiZ::Initialize(PVG::Texture* src_depth_buf, int w, int h, int lvls, bool reproject)
{
	hi_z.SetStorage(GL_TEXTURE_2D, lvls, GL_RG32F, w, h);
	this->lvls = lvls;
	this->w = (w + 7) / 8;
	this->h = (h + 7) / 8;

	PVG::ShaderSource setup(GL_COMPUTE_SHADER), build(GL_COMPUTE_SHADER), build8x8(GL_COMPUTE_SHADER);
	if (reproject)
		setup.SetSourceFile("shaders/hiz/convert_reproj.glsl");
	else
		setup.SetSourceFile("shaders/hiz/convert.glsl");
	setup.Compile();
	hiz_setup.Attach(setup);
	hiz_setup.Link();

	build.SetSourceFile("shaders/hiz/downsample.glsl");
	build.Compile();

	build8x8.SetSourceFile("shaders/hiz/downsample_last.glsl");
	build8x8.Compile();

	hiz_build.Attach(build);
	hiz_build.Link();

	hiz_build8x8.Attach(build8x8);
	hiz_build8x8.Link();

	setup_pipeline.SetShaderProgram(&hiz_setup);
	setup_pipeline.SetTexture(0, src_depth_buf);
	setup_pipeline.SetImage(0, &hi_z, GL_RG32F, GL_WRITE_ONLY, 0);

	build_pipeline.SetShaderProgram(&hiz_build);
	build8x8_pipeline.SetShaderProgram(&hiz_build8x8);
}

void PVG::HiZ::BuildPyramid(ProtoVoxel::Graphics::GpuBuffer* camera_buffer)
{
	hi_z.Clear(0, GL_RG);
	setup_pipeline.SetUBO(0, camera_buffer, 0, sizeof(PVC::GlobalParameters));
	PVG::GraphicsDevice::BindComputePipeline(setup_pipeline);
	PVG::GraphicsDevice::DispatchCompute(w, h, 1);

	build_pipeline.SetUBO(0, camera_buffer, 0, sizeof(PVC::GlobalParameters));
	int w0 = w;
	int h0 = h;
	for (int i = 0; i < lvls; i++) {
		w0 /= 2;
		h0 /= 2;
		if (w0 == 0)
			w0 = 1;
		if (h0 == 0)
			h0 = 1;

		if (w0 == 1 && h0 == 1) {
			build8x8_pipeline.SetImage(0, &hi_z, GL_RG32F, GL_READ_ONLY, i);
			build8x8_pipeline.SetImage(1, &hi_z, GL_RG32F, GL_WRITE_ONLY, i + 1);
			build8x8_pipeline.SetImage(2, &hi_z, GL_RG32F, GL_WRITE_ONLY, i + 2);
			build8x8_pipeline.SetImage(3, &hi_z, GL_RG32F, GL_WRITE_ONLY, i + 3);
			build8x8_pipeline.SetImage(4, &hi_z, GL_RG32F, GL_WRITE_ONLY, i + 4);
			PVG::GraphicsDevice::BindComputePipeline(build8x8_pipeline);
			PVG::GraphicsDevice::DispatchCompute(1, 1, 1);
			break;
		}

		build_pipeline.SetImage(0, &hi_z, GL_RG32F, GL_READ_ONLY, i);
		build_pipeline.SetImage(1, &hi_z, GL_RG32F, GL_WRITE_ONLY, i + 1);
		PVG::GraphicsDevice::BindComputePipeline(build_pipeline);
		PVG::GraphicsDevice::DispatchCompute(w0, h0, 1);
	}
}

PVG::Texture* PVG::HiZ::GetTexture()
{
	return &hi_z;
}
