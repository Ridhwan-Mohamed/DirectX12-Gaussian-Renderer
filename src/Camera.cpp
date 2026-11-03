#include "Camera.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

Camera::Camera(float aspectRatio) noexcept
    : aspectRatio_(std::max(aspectRatio, 0.01f))
{
}

void Camera::SetAspectRatio(float aspectRatio) noexcept
{
    aspectRatio_ = std::max(aspectRatio, 0.01f);
}

void Camera::MoveLocal(
    float right,
    float up,
    float forward) noexcept
{
    XMVECTOR position = XMLoadFloat3(&position_);
    position += RightVector() * right;
    position += XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) * up;
    position += ForwardVector() * forward;
    XMStoreFloat3(&position_, position);
}

void Camera::Rotate(float yawDelta, float pitchDelta) noexcept
{
    yaw_ += yawDelta;
    pitch_ = std::clamp(
        pitch_ + pitchDelta,
        XMConvertToRadians(-89.0f),
        XMConvertToRadians(89.0f));
}

XMMATRIX Camera::ViewMatrix() const noexcept
{
    return XMMatrixLookToLH(
        XMLoadFloat3(&position_),
        ForwardVector(),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

XMMATRIX Camera::ProjectionMatrix() const noexcept
{
    return XMMatrixPerspectiveFovLH(
        verticalFieldOfView_,
        aspectRatio_,
        nearPlane_,
        farPlane_);
}

XMFLOAT3 Camera::Position() const noexcept
{
    return position_;
}

XMVECTOR Camera::ForwardVector() const noexcept
{
    const float cosPitch = std::cos(pitch_);
    return XMVector3Normalize(XMVectorSet(
        std::sin(yaw_) * cosPitch,
        std::sin(pitch_),
        std::cos(yaw_) * cosPitch,
        0.0f));
}

XMVECTOR Camera::RightVector() const noexcept
{
    const XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    return XMVector3Normalize(
        XMVector3Cross(worldUp, ForwardVector()));
}
