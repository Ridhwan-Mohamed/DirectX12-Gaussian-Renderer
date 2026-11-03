#pragma once

#include <DirectXMath.h>

class Camera
{
public:
    explicit Camera(float aspectRatio = 16.0f / 9.0f) noexcept;

    void SetAspectRatio(float aspectRatio) noexcept;
    void MoveLocal(float right, float up, float forward) noexcept;
    void Rotate(float yawDelta, float pitchDelta) noexcept;

    [[nodiscard]] DirectX::XMMATRIX ViewMatrix() const noexcept;
    [[nodiscard]] DirectX::XMMATRIX ProjectionMatrix() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 Position() const noexcept;

private:
    [[nodiscard]] DirectX::XMVECTOR ForwardVector() const noexcept;
    [[nodiscard]] DirectX::XMVECTOR RightVector() const noexcept;

    DirectX::XMFLOAT3 position_{0.0f, 0.0f, -3.0f};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float aspectRatio_ = 16.0f / 9.0f;
    float verticalFieldOfView_ = DirectX::XMConvertToRadians(60.0f);
    float nearPlane_ = 0.01f;
    float farPlane_ = 10'000.0f;
};
