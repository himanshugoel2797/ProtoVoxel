#pragma once
#include <stdint.h>
#include <memory>
#include "texture.h"
#include "computepipeline.h"

namespace ProtoVoxel::Graphics {
	class HiZ {
	private:
		ProtoVoxel::Graphics::Texture hi_z;

		ProtoVoxel::Graphics::ShaderProgram hiz_setup;
		ProtoVoxel::Graphics::ShaderProgram hiz_build;
		ProtoVoxel::Graphics::ShaderProgram hiz_build8x8;

		ProtoVoxel::Graphics::ComputePipeline setup_pipeline;
		ProtoVoxel::Graphics::ComputePipeline build_pipeline;
		ProtoVoxel::Graphics::ComputePipeline build8x8_pipeline;

		int lvls, w, h;
	public:
		HiZ();
		~HiZ();

		void Initialize(ProtoVoxel::Graphics::Texture* src_depth_buf, int w, int h, int lvls, bool reproject = true);
		void BuildPyramid(ProtoVoxel::Graphics::GpuBuffer* camera_buffer);
		ProtoVoxel::Graphics::Texture* GetTexture();
	};
}