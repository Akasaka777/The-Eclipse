#include "Core/Camera.h"

#include "Common/MathUtil.h"
#include "Core/GameConfig.h"

namespace ecl {

namespace {
const float kHalfW = static_cast<float>(config::kScreenWidth) * 0.5f;
const float kHalfH = static_cast<float>(config::kScreenHeight) * 0.5f;
} // namespace

void Camera::Reset()
{
    position_ = Vec2(kHalfW, kHalfH);
    target_ = position_;
    shakeOffset_ = Vec2(0.0f, 0.0f);
    shakePower_ = 0.0f;
    shakeTimer_ = 0.0f;
    shakeDuration_ = 0.0f;
}

void Camera::SetWorldBounds(float minX, float maxX, float minY, float maxY)
{
    minX_ = minX;
    maxX_ = maxX;
    minY_ = minY;
    maxY_ = maxY;
}

void Camera::SnapToTarget()
{
    position_ = target_;
    // 境界クランプ
    const float left = math::MinF(minX_ + kHalfW, maxX_ - kHalfW);
    const float right = math::MaxF(minX_ + kHalfW, maxX_ - kHalfW);
    position_.x = math::Clamp(position_.x, left, right);
    const float top = math::MinF(minY_ + kHalfH, maxY_ - kHalfH);
    const float bottom = math::MaxF(minY_ + kHalfH, maxY_ - kHalfH);
    position_.y = math::Clamp(position_.y, top, bottom);
}

void Camera::Update(float dt)
{
    const float t = math::DampFactor(followSpeed_, dt);
    position_.x += (target_.x - position_.x) * t;
    position_.y += (target_.y - position_.y) * t;

    const float left = math::MinF(minX_ + kHalfW, maxX_ - kHalfW);
    const float right = math::MaxF(minX_ + kHalfW, maxX_ - kHalfW);
    position_.x = math::Clamp(position_.x, left, right);
    const float top = math::MinF(minY_ + kHalfH, maxY_ - kHalfH);
    const float bottom = math::MaxF(minY_ + kHalfH, maxY_ - kHalfH);
    position_.y = math::Clamp(position_.y, top, bottom);

    if (shakeTimer_ > 0.0f) {
        shakeTimer_ -= dt;
        const float ratio = (shakeDuration_ > 0.0f) ? math::MaxF(shakeTimer_ / shakeDuration_, 0.0f) : 0.0f;
        const float power = shakePower_ * ratio;
        shakeOffset_.x = math::RandFloat(-power, power);
        shakeOffset_.y = math::RandFloat(-power, power);
        if (shakeTimer_ <= 0.0f) {
            shakeOffset_ = Vec2(0.0f, 0.0f);
            shakePower_ = 0.0f;
        }
    }
}

void Camera::Shake(float power, float duration)
{
    if (!shakeEnabled_) return;
    // 既存の振動より強い場合のみ上書き
    if (power >= shakePower_ * 0.8f) {
        shakePower_ = power;
        shakeDuration_ = duration;
        shakeTimer_ = duration;
    }
}

float Camera::ViewLeft() const
{
    return position_.x - kHalfW + shakeOffset_.x;
}

float Camera::ViewTop() const
{
    return position_.y - kHalfH + shakeOffset_.y;
}

Vec2 Camera::WorldToScreen(const Vec2& world) const
{
    return Vec2(world.x - ViewLeft(), world.y - ViewTop());
}

Rect Camera::WorldToScreen(const Rect& world) const
{
    const float dx = ViewLeft();
    const float dy = ViewTop();
    return Rect(world.left - dx, world.top - dy, world.right - dx, world.bottom - dy);
}

Vec2 Camera::ScreenToWorld(const Vec2& screen) const
{
    return Vec2(screen.x + ViewLeft(), screen.y + ViewTop());
}

bool Camera::IsVisible(const Rect& world, float margin) const
{
    const Rect view(ViewLeft() - margin, ViewTop() - margin,
                    ViewLeft() + static_cast<float>(config::kScreenWidth) + margin,
                    ViewTop() + static_cast<float>(config::kScreenHeight) + margin);
    return view.Intersects(world);
}

} // namespace ecl
