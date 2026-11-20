//==============================================================================
// Camera.h : 横スクロール用カメラ（追従 / 境界クランプ / 画面振動）
//   ワールドの端に近づくと追従をゆるめ、端でいきなり止まらず減速して止まる
//==============================================================================
#pragma once

#include "Common/Types.h"

namespace ecl {

// 端の手前でカメラを減速させる幅（px）。追従の目標が端の内側この距離に入ると寄せ方をゆるめ、
// 端より外側にこの距離出たところでちょうど端に着く（歩きなら約 1 秒かけて止まる）。0 で端でぴたりと止まる
constexpr float kCameraEdgeEase = 240.0f;

// 目標 value を [lo, hi] に収める。端から ease の範囲では端に近いほど動きを小さくする
//   （端の内側 ease の位置から寄せ始め、端の外側 ease の位置でちょうど端に着く。動く割合は 1 → 0 へなめらかに変わる）
float EaseIntoRange(float value, float lo, float hi, float ease);

class Camera
{
public:
    void Reset();
    // スクロール可能なワールド範囲
    void SetWorldBounds(float minX, float maxX, float minY, float maxY);
    void SetTarget(const Vec2& worldPos) { target_ = worldPos; }
    void SetFollowSpeed(float speed) { followSpeed_ = speed; }
    // 端の手前で減速させる幅（0 で端でぴたりと止まる）
    void SetEdgeEase(float width) { edgeEase_ = (width > 0.0f) ? width : 0.0f; }
    float EdgeEase() const { return edgeEase_; }
    // 目標へ一気に合わせる。eased = false なら端の手前で寄せず、目標をそのまま範囲に収める
    // （ボスの登場演出など、決めた位置に正確に置きたいとき）
    void SnapToTarget(bool eased = true);
    // 追従を進める。follow = false なら位置は動かさず、画面振動だけ進める
    void Update(float dt, bool follow = true);
    // 追従で実際に目指す位置（端の手前で寄せたもの）
    Vec2 FollowGoal() const;

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
    float edgeEase_ = kCameraEdgeEase;
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
