#include <cuda.h>
#include <cooperative_groups.h>
#include <cooperative_groups/reduce.h>
#define GLM_FORCE_CUDA
#include "glm/glm.hpp"
#include "chunkmanager.h"

namespace PPC = ProtoVoxel::PointCloud;
namespace cg = cooperative_groups;

typedef struct
{
        glm::mat4 proj;
        glm::mat4 view;
        glm::mat4 vp;
        glm::mat4 ivp;
        glm::mat4 prev_proj;
        glm::mat4 prev_view;
        glm::mat4 prev_vp;
        glm::mat4 prev_ivp;
        glm::vec4 prev_eyePos;
        glm::vec4 prev_eyeUp;
        glm::vec4 prev_eyeDir;
        glm::vec4 eyePos;
        glm::vec4 eyeUp;
        glm::vec4 eyeDir;
        glm::vec4 eyeRight;
} globalParams_t;

__device__ __forceinline__ int floatToOrderedInt(float floatVal) {
    int intVal = __float_as_int(floatVal);
    return (intVal >= 0) ? intVal : intVal ^ 0x7FFFFFFF;
}
__device__ __forceinline__ float orderedIntToFloat(int intVal) {
    return __int_as_float((intVal >= 0) ? intVal : intVal ^ 0x7FFFFFFF);
}

__global__ void splat_kernel(void* global_vars, glm::uvec4* splat_points, uint32_t* pointBuffer, cudaSurfaceObject_t colorBuffer, int w, int h, int draw_count)
{

    int ptIdx = blockIdx.x * blockDim.x + threadIdx.x;
    globalParams_t* globalParams = (globalParams_t*)global_vars;

    if(ptIdx >= draw_count)
        return;

    glm::uvec4 data = splat_points[ptIdx];

    glm::vec4 ppos = glm::vec4(data.x / (float)POS_SCALE_FACTOR, data.y / (float)POS_SCALE_FACTOR, data.z / (float)POS_SCALE_FACTOR, 1.0f);
    uchar4 pcol = make_uchar4((data.w & 0xff), ((data.w >> 8) & 0xff), ((data.w >> 16) & 0xff), ((data.w >> 24) & 0xff));

    ppos = globalParams->vp * ppos;
    ppos /= ppos.w;
    
    ppos.x = (ppos.x + 1.0f) * 0.5f;
    ppos.y = (ppos.y + 1.0f) * 0.5f;
    glm::ivec2 ppos_pxl = glm::ivec2(__float2int_rd(ppos.x * w), __float2int_rd(ppos.y * h));

    if (ppos_pxl.x < 0)
        return;
    if (ppos_pxl.x >= w)
        return;
    if (ppos_pxl.y < 0.0f)
        return;
    if (ppos_pxl.y >= h)
        return;
    if (ppos.z < 0)
        return;

    int cur_depth = floatToOrderedInt(ppos.z);

    uint32_t pointBufferIdx = ppos_pxl.y * w + ppos_pxl.x;
    //get all subgroups where the point is in the same pixel

    unsigned int mask = __match_any_sync(__activemask(), pointBufferIdx);
    /*if (__popc(mask) <= 1)
    {
        if(cur_depth > atomicMax(&pointBuffer[pointBufferIdx], cur_depth))
        {
            surf2Dwrite(pcol, colorBuffer, ppos_pxl.x * sizeof(uchar4), ppos_pxl.y);
        }
    }else*/
    {
        if(cur_depth == __reduce_max_sync(mask, cur_depth))
        {
            if(cur_depth > atomicMax(&pointBuffer[pointBufferIdx], cur_depth))
            {
                //pcol = make_uchar4((int)(ppos.z / 0.01f * 255), (int)(ppos.z / 0.01f * 255), (int)(ppos.z / 0.01f * 255), (int)(ppos.z / 0.01f * 255));
                //printf("%d %f\n", mask, ppos.z);
                surf2Dwrite(pcol, colorBuffer, ppos_pxl.x * sizeof(uchar4), ppos_pxl.y);
            }
        }
    }

}

void PPC::ChunkManager::splat(void* global_vars, void* splat_points, void* pointBuffer, cudaSurfaceObject_t colorBuffer, int w, int h, int draw_count)
{
    int threadsPerBlock = 256;
    int blocksPerGrid = (draw_count + threadsPerBlock - 1) / threadsPerBlock;
    splat_kernel<<<blocksPerGrid, threadsPerBlock>>>(global_vars, (glm::uvec4*)splat_points, (uint32_t*)pointBuffer, colorBuffer, w, h, draw_count);
}