#include "core/input.h"
#include "core/window.h"
#include "graphics/gpubuffer.h"
#include "graphics/graphicsdevice.h"
#include "scenegraph/scenebase.h"
#include "pointcloud/chunkmanager.h"
#include "pointcloud/chunkjobmanager.h"

#include "misc/model.h"

#include "imgui/imgui.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <immintrin.h>

namespace PVC = ProtoVoxel::Core;
namespace PVG = ProtoVoxel::Graphics;
namespace PPC = ProtoVoxel::PointCloud;
namespace PVSG = ProtoVoxel::SceneGraph;
namespace PVM = ProtoVoxel::Misc;

class TestScene : public PVSG::SceneBase
{
private:
public:
    TestScene() {}
    ~TestScene() {}

    PPC::ChunkManager manager;
    
    void Initialize() override
    {
        PVSG::SceneBase::Initialize();

        manager.Initialize();
    }

    void Update(double time) override
    {
        manager.Update(camera.GetParameters()->eyePos, camera.GetParameters()->vp, PVSG::SceneBase::camera_buffer.GetBuffer());
        PVSG::SceneBase::Update(time);
    }

    void Render(double time) override
    {
        manager.Render(PVSG::SceneBase::camera_buffer.GetBuffer(), time);

        auto tmp = camera.GetParameters()->eyePos;
        
        ImGui::Begin("Camera Info");
        ImGui::Text("Camera Position: %f, %f, %f\n", tmp.x, tmp.y, tmp.z);
        tmp = camera.GetParameters()->eyeDir;
        ImGui::Text("Camera Direction: %f, %f, %f\n", tmp.x, tmp.y, tmp.z);
        ImGui::End();
    }
};

int main(int, char **)
{
    PVC::Window win(1024, 1024, "Test");
    win.InitGL();

    PVC::Input::RegisterWindow(&win);

    TestScene* scene = new TestScene();
    scene->Initialize();

    srand(0);

    double lastTime = 0, nowTime = 0;
    while (!win.ShouldClose())
    {
        PVG::GraphicsDevice::ClearAll();
        win.StartFrame();
        auto delta = (nowTime - lastTime) * 1000;

        scene->UpdateStart(delta);
        scene->Update(delta);
        scene->Render(delta);

        win.SwapBuffers();
        lastTime = nowTime;
        nowTime = win.GetTime();
    }
}
