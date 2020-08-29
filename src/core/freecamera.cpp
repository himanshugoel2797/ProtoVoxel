#include "freecamera.h"
#include "glm/gtc/matrix_transform.hpp"

namespace PVC = ProtoVoxel::Core;

float rotationSpeed = 0.2f;
float moveSpeed = 0.5f;

glm::mat4 MakeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float zNear)
{
    float f = 1.0f / tan(fovY_radians / 2.0f);
    return glm::mat4(
        f / aspectWbyH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, zNear, 0.0f);
}

PVC::FreeCamera::FreeCamera()
{
    UpBinding = PVC::Input::RegisterBinding(GLFW_KEY_UP);
    DownBinding = PVC::Input::RegisterBinding(GLFW_KEY_DOWN);
    LeftBinding = PVC::Input::RegisterBinding(GLFW_KEY_LEFT);
    RightBinding = PVC::Input::RegisterBinding(GLFW_KEY_RIGHT);
    ForwardBinding = PVC::Input::RegisterBinding(GLFW_KEY_PAGE_UP);
    BackwardBinding = PVC::Input::RegisterBinding(GLFW_KEY_PAGE_DOWN);
    AccelerateBinding = PVC::Input::RegisterBinding(GLFW_KEY_HOME);
    DecelerateBinding = PVC::Input::RegisterBinding(GLFW_KEY_END);

    pos = glm::vec3(-1, 0, 0);
    dir = glm::vec3(1, 0, 0);
    up = glm::vec3(0, 1, 0);

    params.eyePos = glm::vec4(pos, 1);
    params.eyeDir = glm::vec4(dir, 0);
    params.eyeUp = glm::vec4(up, 0);
    params.prev_proj = params.proj = MakeInfReversedZProjRH(glm::radians(45.0f), 9.0f / 9.0f, 0.01f);
}

PVC::FreeCamera::~FreeCamera() {}

struct PVC::GlobalParameters *PVC::FreeCamera::GetParameters()
{
    return &params;
}

glm::mat4 PVC::FreeCamera::UpdateViewMatrix()
{
    glm::mat4 cameraRotation(1.0f);
    cameraRotation = glm::rotate(cameraRotation, updownRot, glm::vec3(1, 0, 0));
    cameraRotation = glm::rotate(cameraRotation, leftrightRot, glm::vec3(0, 1, 0));

    glm::vec4 cameraOriginalTarget(0, 0, 1, 0);
    glm::vec4 cameraOriginalUpVector(0, 1, 0, 0);

    dir = cameraRotation * cameraOriginalTarget;
    glm::vec3 cameraFinalTarget = pos + dir;

    camRotUp = cameraRotation * cameraOriginalUpVector;

    return glm::lookAt(pos, cameraFinalTarget, camRotUp);
}

void PVC::FreeCamera::Update(double time)
{
    params.prev_eyeDir = params.eyeDir;
    params.prev_eyeUp = params.eyeUp;
    params.prev_eyePos = params.eyePos;
    params.prev_view = params.view;
    params.prev_ivp = params.ivp;
    params.prev_vp = params.vp;

    glm::vec2 latestMousePos;
    PVC::Input::GetMousePosition(latestMousePos);

    if (PVC::Input::IsMouseDown(PVC::MouseButton::Left))
    {
        if (abs(mousePos.x - latestMousePos.x) > 0)
            leftrightRot -= (float)glm::radians(rotationSpeed * (mousePos.x - latestMousePos.x) * time / 1000.0f);
        if (abs(mousePos.y - latestMousePos.y) > 0)
            updownRot -= (float)glm::radians(rotationSpeed * (mousePos.y - latestMousePos.y) * time / 1000.0f);
    }
    else
    {
        mousePos = latestMousePos;
    }
    UpdateViewMatrix();
    glm::vec3 Right = glm::cross(camRotUp, dir);

    if (PVC::Input::IsKeyDown(ForwardBinding))
    {
        pos += dir * (float)(moveSpeed * time / 1000.0f);
    }
    else if (PVC::Input::IsKeyDown(BackwardBinding))
    {
        pos -= dir * (float)(moveSpeed * time / 1000.0f);
    }

    if (PVC::Input::IsKeyDown(LeftBinding))
    {
        pos -= Right * (float)(moveSpeed * time / 1000.0f);
    }
    else if (PVC::Input::IsKeyDown(RightBinding))
    {
        pos += Right * (float)(moveSpeed * time / 1000.0f);
    }

    //#if DEBUG
    if (PVC::Input::IsKeyDown(DownBinding))
    {
        pos += camRotUp * (float)(moveSpeed * time / 1000.0f);
    }
    else if (PVC::Input::IsKeyDown(UpBinding))
    {
        pos -= camRotUp * (float)(moveSpeed * time / 1000.0f);
    }

    if (PVC::Input::IsKeyDown(AccelerateBinding))
    {
        moveSpeed += 0.02f * moveSpeed;
    }
    else if (PVC::Input::IsKeyDown(DecelerateBinding))
    {
        moveSpeed -= 0.02f * moveSpeed;
    }
    //#endif

    params.eyeDir = glm::vec4(dir, 0);
    params.eyeUp = glm::vec4(up, 0);
    params.eyePos = glm::vec4(pos, 1);
    params.view = glm::lookAt(pos, pos + dir, camRotUp);
    params.vp = params.proj * params.view;
    params.ivp = glm::inverse(params.vp);
    params.eyeRight = glm::vec4(Right, 0);

    //Engine.Frustum = new Frustum(Engine.View, Engine.Projection, pos);
}