#include "core/window.h"
#include <iostream>
#include <random>
#include <chrono>

#include "voxel/chunk.h"
#include "voxel/chunk_malloc.h"

#include <windows.h>

namespace PVC = ProtoVoxel::Core;
namespace PVV = ProtoVoxel::Voxel;

int main(int, char **)
{
    PVC::Window win(640, 480, "Test");
    win.InitGL();

    double avg_duration = 0;
    int sample_cnt = 10000;
    int iter_cnt = 30000;

    //SYSTEM_INFO sysinfo;
    //GetSystemInfo(&sysinfo);
    //std::cout << "Page Granularity: " << sysinfo.dwAllocationGranularity << std::endl;

    srand(0);

    auto x_v_2 = new uint8_t[sample_cnt * iter_cnt * 3];
    for (int i = 0; i < sample_cnt * iter_cnt; i++)
    {
        uint8_t x = rand() % 32;
        uint8_t y = rand() % 32;
        uint8_t z = rand() % 32;

        x_v_2[i * 3 + 0] = x;
        x_v_2[i * 3 + 1] = y;
        x_v_2[i * 3 + 2] = z;
    }

    PVV::ChunkMalloc chnk_malloc;
    //chnk_malloc.Initialize();

    auto x_v = (uint8_t *)x_v_2;
    for (int samples = 0; samples < sample_cnt; samples++)
    {
        PVV::Chunk chnk(&chnk_malloc);

        auto start_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < iter_cnt; i++)
        {
            uint8_t x = *x_v++;
            uint8_t y = *x_v++;
            uint8_t z = *x_v++;

            chnk.SetSingle(x, y, z, 1);
        }
        auto stop_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        avg_duration += (stop_ns - start_ns) / 1000.0;
    }
    std::cout << "[SetSingle] Duration: " << avg_duration / sample_cnt << "us, Per Insert: " << avg_duration / (sample_cnt * iter_cnt) * 1000 << "ns" << std::endl;

    avg_duration = 0;
    x_v = (uint8_t *)x_v_2;
    for (int samples = 0; samples < sample_cnt; samples++)
    {
        PVV::Chunk chnk(&chnk_malloc);

        auto start_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < iter_cnt; i++)
        {
            uint8_t x = *x_v++;
            uint8_t y = *x_v++;
            uint8_t z = *x_v++;

            chnk.SetSingle(x, y, z, 1);
        }

        auto resvec = new uint32_t[chnk.GetCompiledLen()];
        chnk.Compile(resvec);
        auto stop_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        delete[] resvec;

        avg_duration += (stop_ns - start_ns) / 1000.0;
    }
    std::cout << "[SetSingle + UpdateMasks] Duration: " << avg_duration / sample_cnt << "us, Per Insert: " << avg_duration / (sample_cnt * iter_cnt) * 1000 << "ns" << std::endl;

    while (!win.ShouldClose())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        win.StartFrame();
        win.SwapBuffers();
    }
}
