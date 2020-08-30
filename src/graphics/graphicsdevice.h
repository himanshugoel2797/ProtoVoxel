#pragma once
#include "computepipeline.h"
#include "framebuffer.h"
#include "gpubuffer.h"
#include "graphicspipeline.h"
#include "shaderprogram.h"

#include <memory>

namespace ProtoVoxel::Graphics
{
    enum class Topology
    {
        Triangles,
        TriangleStrip,
        Points,
        Lines,
        LineStrip,
    };

    enum class IndexType
    {
        UByte,
        UShort,
        UInt,
    };

    class GraphicsDevice
    {
    private:
    public:
        static const int MAX_BINDPOINTS = 1024;

        static void ClearColor();
        static void ClearDepth();
        static void ClearAll();

        static void BindGraphicsPipeline(GraphicsPipeline const &pipeline);
        static void BindComputePipeline(ComputePipeline const &pipeline);

        static void MultiDrawIndirect(Topology topo, int cmdOffset, int stride, int draw_count);
        static void MultiDrawIndirectCount(Topology topo, int cmdOffset, int drawOffset, int stride, int draw_count);
        static void DispatchCompute(uint32_t x, uint32_t y, uint32_t z);
        static void MultiDrawElementsIndirectCount(Topology topo, IndexType type, int cmdOffset, int drawOffset, int stride, int maxDrawCount);
        static void MultiDrawElementsIndirect(Topology topo, IndexType type, int cmdOffset, int stride, int maxDrawCount);
    };
} // namespace ProtoVoxel::Graphics