#pragma once
#include <stdint.h>
#include <cmath>

#include "input.h"
#include "glm/glm.hpp"

namespace ProtoVoxel::Core
{
    struct GlobalParameters
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
    };

    class FreeCamera
    {
    private:
        glm::vec3 angularVel;
        glm::vec3 vel;
        float height;
        glm::vec3 pos;
        glm::vec3 dir;
        glm::vec3 up;
        glm::vec3 camRotUp;

        glm::vec2 mousePos;

        float leftrightRot = 0;
        float updownRot = 0;

        int UpBinding;
        int DownBinding;
        int LeftBinding;
        int RightBinding;
        int ForwardBinding;
        int BackwardBinding;
        int AccelerateBinding;
        int DecelerateBinding;

        glm::mat4 UpdateViewMatrix();
        struct GlobalParameters params;

    public:
        FreeCamera();
        ~FreeCamera();

        struct GlobalParameters *GetParameters();
        void Update(double time);
    };
} // namespace ProtoVoxel::Core