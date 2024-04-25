#include "Camera.h"

void Camera::SetCamera(int width, int height, vec3 position)
{
    this->width = width;
    this->height = height;
    this->position = position;
}

void Camera::Matrix(float FOVdeg, float nearPlane, float farPlane)
{
    view = glm::lookAt(position, position + direction, up);
    projection = glm::perspective(glm::radians(FOVdeg), (float)(width / height), nearPlane, farPlane);
}

void Camera::UpdateCameraAspectRatio(int width, int height)
{
    this->width = width;
    this->height = height;
}