#include "core/window.h"
#include <iostream>
#include <random>
#include <chrono>

#include "voxel/chunk.h"

namespace PVC = ProtoVoxel::Core;
namespace PVV = ProtoVoxel::Voxel;

int main(int, char **)
{
    PVC::Window win(640, 480, "Test");
    win.InitGL();

    double avg_duration = 0;
    int sample_cnt = 10000;
    int iter_cnt = 30000;

    auto x_v = new uint8_t[iter_cnt];
    auto y_v = new uint8_t[iter_cnt];
    auto z_v = new uint8_t[iter_cnt];
    for (int i = 0; i < iter_cnt; i++)
    {
        uint8_t x = rand() % 30;
        uint8_t y = rand() % 30;
        uint8_t z = rand() % 30;

        x_v[i] = x + 1;
        y_v[i] = y + 1;
        z_v[i] = z + 1;
    }

    for (int samples = 0; samples < sample_cnt; samples++)
    {
        PVV::Chunk chnk;

        auto start_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < iter_cnt; i++)
        {
            uint8_t x = x_v[i];
            uint8_t y = y_v[i];
            uint8_t z = z_v[i];

            chnk.SetSingle(x, y, z, 1);
        }
        auto stop_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        avg_duration += (stop_ns - start_ns) / 1000.0;
    }
    std::cout << "Duration: " << avg_duration / sample_cnt << "us, Per Insert: " << avg_duration / (sample_cnt * iter_cnt) * 1000 << "ns" << std::endl;

    while (!win.ShouldClose())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        win.StartFrame();
        win.SwapBuffers();
    }
}
