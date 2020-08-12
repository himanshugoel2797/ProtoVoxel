#include "core/window.h"
#include <iostream>
#include <random>
#include <chrono>

#include "voxel/mortoncode.h"

namespace PVC = ProtoVoxel::Core;
namespace PVV = ProtoVoxel::Voxel;

int main(int, char **) {
    PVC::Window win(640, 480, "Test");
    win.InitGL();

    double netTime = 0;

    int runs = 10000;
    int iter_cnt = 100000;
    uint8_t iter_table[iter_cnt][3];
    uint32_t iter_Table_u[iter_cnt];
    for (int iters = 0; iters < iter_cnt; iters++) {
        iter_table[iters][0] = (uint8_t)rand();
        iter_table[iters][1] = (uint8_t)rand();
        iter_table[iters][2] = (uint8_t)rand();

        //iter_Table_u[iters] = PVV::MortonCode::Encode(iter_table[iters][0], iter_table[iters][1], iter_table[iters][2]);
    }

    for (int samples = 0; samples < runs; samples++) {
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        for (int iters = 0; iters < iter_cnt; iters++) {
            auto res = PVV::MortonCode::Encode(iter_table[iters][0], iter_table[iters][1], iter_table[iters][2]);
            iter_Table_u[iters] = res;
        }

        auto finish = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        netTime += (finish - start) / 1000.0;
    }
    std::cout << "Average Time: " << netTime / runs << "us" << std::endl;

    while (!win.ShouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT);
        win.StartFrame();
        win.SwapBuffers();
    }
}
