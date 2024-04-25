#pragma once

#include "Globals.h"

class Camera
{
public:

    Camera() {}

    glm::vec3 position;
    glm::vec3 direction = vec3(-5, -5, -5);
    glm::vec3 up = vec3(0.0f, 1.0f, 0.0f);

    int width;
    int height;

    float speed = 0.1f;

    glm::mat4 view;
    glm::mat4 projection;

    void SetCamera(int width, int height, vec3 position);

    void Matrix(float FOVdeg, float nearPlane, float farPlane);

    void UpdateCameraAspectRatio(int width, int height);
};