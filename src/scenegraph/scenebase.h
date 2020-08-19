#pragma once

#include "../core/freecamera.h"
#include "../graphics/shadowed_gpubuffer.h"

namespace ProtoVoxel::SceneGraph
{
    class SceneBase
    {
    private:
    public:
        ProtoVoxel::Graphics::ShadowedGpuBuffer camera_buffer;
        ProtoVoxel::Core::FreeCamera camera;

        SceneBase();
        ~SceneBase();

        virtual void Initialize();
        void UpdateStart(double time);
        virtual void Update(double time);
        virtual void Render(double time);
    };
} // namespace ProtoVoxel::SceneGraph