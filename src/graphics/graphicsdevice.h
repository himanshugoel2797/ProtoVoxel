#pragma once
#include "computepipeline.h"
#include "framebuffer.h"
#include "gpubuffer.h"
#include "graphicspipeline.h"
#include "shaderprogram.h"

#include <memory>

namespace ProtoVoxel::Graphics {
class GraphicsDevice {
private:
public:
  static const int MAX_BINDPOINTS = 1024;

  static void ClearColor();
  static void ClearDepth();
  static void ClearAll();

  static void BindGraphicsPipeline(GraphicsPipeline const &pipeline);
  static void BindComputePipeline(ComputePipeline const &pipeline);

  static void MultiDrawIndirect();
  static void MulitDrawElementsIndirect();
};
} // namespace ProtoVoxel::Graphics