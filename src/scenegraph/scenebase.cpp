#include "scenebase.h"

namespace PVSG = ProtoVoxel::SceneGraph;
namespace PVG = ProtoVoxel::Graphics;
namespace PVC = ProtoVoxel::Core;

PVSG::SceneBase::SceneBase() : camera(), camera_buffer()
{
}

PVSG::SceneBase::~SceneBase()
{
}

void PVSG::SceneBase::Initialize()
{
    camera_buffer.SetStorage(sizeof(PVC::GlobalParameters), GL_DYNAMIC_STORAGE_BIT);
}

void PVSG::SceneBase::UpdateStart(double time)
{
    camera.Update(time);
    camera_buffer.Update(0, sizeof(PVC::GlobalParameters), camera.GetParameters());
}

void PVSG::SceneBase::Update(double time)
{
    camera_buffer.Swap();
}

void PVSG::SceneBase::Render(double time)
{
}