//==============================================================================
// Camera.h : 横スクロール用カメラ（追従 / 境界クランプ / 画面振動）
//==============================================================================
#pragma once

#include "Common/Types.h"

namespace ecl {

class Camera
{
public:
    void Reset();
    // スクロール可能なワールド範囲
    void SetWorldBounds(float minX, float maxX, float minY, float maxY);
    void SetTarget(const Vec2& worldPos) { target_ = worldPos; }
    void SetFollowSpeed(float speed) { followSpeed_ = speed; }
    void SnapToTarget();
    void Update(float dt);

    // 画面振動
    void Shake(float power, float duration);
    void SetShakeEnabled(bool enabled) { shakeEnabled_ = enabled; }

    Vec2 WorldToScreen(const Vec2& world) const;
    Rect WorldToScreen(const Rect& world) const;
    Vec2 ScreenToWorld(const Vec2& screen) const;

    // ビュー左上のワールド座標
    float ViewLeft() const;
    float ViewTop() const;
    // 画面外カリング用
    bool IsVisible(const Rect& world, float margin = 128.0f) const;

    const Vec2& Center() const { return position_; }

private:
    Vec2  position_;      // ビュー中心
    Vec2  target_;
    Vec2  shakeOffset_;
    float followSpeed_ = 8.0f;
    float shakePower_ = 0.0f;
    float shakeTimer_ = 0.0f;
    float shakeDuration_ = 0.0f;
    bool  shakeEnabled_ = true;

    float minX_ = 0.0f;
    float maxX_ = 0.0f;
    float minY_ = 0.0f;
    float maxY_ = 0.0f;
};

} // namespace ecl
