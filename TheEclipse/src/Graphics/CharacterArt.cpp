#include "Graphics/CharacterArt.h"

#include "Common/MathUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Game/WeaponMotion.h"
#include "Graphics/DrawUtil.h"

#include <cmath>
#include <utility>
#include <vector>

namespace ecl {

namespace {

constexpr float kTwoPi = 6.28318530718f;

// 手足の描画（太い線＋関節）
void Limb(const Vec2& from, const Vec2& to, float thickness, const ColorRGB& color, int alpha)
{
    draw::Line(from.x, from.y, to.x, to.y, color, thickness, alpha);
    draw::Circle(to.x, to.y, thickness * 0.5f, color, true, 1.0f, alpha);
}

Vec2 Rotate(const Vec2& origin, float length, float angle)
{
    return Vec2(origin.x + std::cos(angle) * length, origin.y + std::sin(angle) * length);
}

// 待機・移動系のポーズにおける武器の角度 : 0 = 正面水平 / 90 = 真下
//   攻撃・スキルの振り方は WeaponMotion 側（武器種ごとの定義）に任せる
float RestingWeaponDeg(PoseKind pose, float phase)
{
    switch (pose) {
    case PoseKind::Idle:  return 70.0f + std::sin(phase * kTwoPi) * 4.0f;
    case PoseKind::Walk:  return 65.0f + std::sin(phase * kTwoPi) * 12.0f;
    case PoseKind::Run:   return 55.0f + std::sin(phase * kTwoPi) * 18.0f;
    case PoseKind::Jump:  return 35.0f;
    case PoseKind::Fall:  return 50.0f;
    case PoseKind::Dash:  return 95.0f;
    case PoseKind::Guard: return 80.0f;
    case PoseKind::Hurt:  return 110.0f;
    case PoseKind::Dead:  return 95.0f;
    default:              return 70.0f;
    }
}

// 角度をラジアンへ（facing = -1 なら左右反転する）
float ToWeaponAngle(float deg, int facing)
{
    const float rad = math::DegToRad(deg);
    return (facing >= 0) ? rad : (math::DegToRad(180.0f) - rad);
}

// ポーズに対応する武器モーションを取り出す
WeaponSwing SampleSwing(PoseKind pose, float phase, const ActorArt& art, int variant)
{
    if (pose == PoseKind::Attack) return SampleComboSwing(art.weapon, variant, phase);
    if (pose == PoseKind::Skill) return SampleSkillSwing(art.weapon, variant, phase);

    WeaponSwing swing;
    swing.angleDeg = RestingWeaponDeg(pose, phase);
    return swing;
}

//------------------------------------------------------------------------------
// 人型（プレイヤー / 騎士 / 小型）の代替描画
//------------------------------------------------------------------------------
void DrawHumanoid(const Rect& rect, int facing, PoseKind pose, float phase,
                  const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent,
                  int variant)
{
    const float w = rect.Width();
    const float h = rect.Height();
    const float cx = rect.CenterX();
    const float bottom = rect.bottom;
    const float dir = (facing >= 0) ? 1.0f : -1.0f;

    // --- 姿勢パラメータ -----------------------------------------------------
    float lean = 0.0f;   // 前傾（px）
    float crouch = 0.0f; // 沈み込み（px）
    float legSwing = 0.0f;

    switch (pose) {
    case PoseKind::Idle:
        crouch = std::sin(phase * kTwoPi) * h * 0.012f;
        break;
    case PoseKind::Walk:
        legSwing = std::sin(phase * kTwoPi) * h * 0.14f;
        lean = dir * h * 0.02f;
        break;
    case PoseKind::Run:
        legSwing = std::sin(phase * kTwoPi) * h * 0.22f;
        lean = dir * h * 0.07f;
        crouch = h * 0.02f;
        break;
    case PoseKind::Jump:
        legSwing = h * 0.10f;
        lean = dir * h * 0.03f;
        break;
    case PoseKind::Fall:
        legSwing = -h * 0.08f;
        break;
    case PoseKind::Dash:
        lean = dir * h * 0.16f;
        crouch = h * 0.06f;
        break;
    case PoseKind::Guard:
        crouch = h * 0.06f;
        lean = -dir * h * 0.02f;
        break;
    case PoseKind::Hurt:
        lean = -dir * h * 0.12f;
        crouch = h * 0.04f;
        break;
    case PoseKind::Dead:
        break;
    default:
        break;
    }

    // --- 武器モーション（攻撃・スキルは武器種ごとの定義で上書きする）-----------
    const bool swinging = (pose == PoseKind::Attack || pose == PoseKind::Skill);
    const WeaponSwing swing = SampleSwing(pose, phase, art, variant);
    if (swinging) {
        lean = dir * h * swing.lean;
        crouch = h * swing.crouch;
        // 踏み込みに合わせて足を開く
        legSwing = dir * h * swing.lean * 0.9f;
    }

    // --- 死亡時は横たわった表現 ---------------------------------------------
    if (pose == PoseKind::Dead) {
        const float fall = math::Clamp(phase, 0.0f, 1.0f);
        const float bodyH = math::Lerp(h, h * 0.28f, fall);
        const float bodyW = math::Lerp(w * 0.6f, w * 1.25f, fall);
        const Rect body = Rect::FromFoot(cx, bottom, bodyW * 0.5f, bodyH);
        draw::FillRect(body, main, alpha);
        draw::StrokeRect(body, accent.Scaled(0.6f), 2.0f, alpha);
        draw::Circle(cx - dir * bodyW * 0.35f, bottom - bodyH * 0.5f, h * 0.10f, accent, true, 1.0f, alpha);
        return;
    }

    const float hipY = bottom - h * 0.46f + crouch;
    const float shoulderY = bottom - h * 0.74f + crouch;
    const float headR = h * 0.105f;
    const float headY = bottom - h * 0.84f + crouch - headR * 0.4f;
    const float torsoW = w * 0.52f;

    // --- 脚 -----------------------------------------------------------------
    const Vec2 hip(cx + lean * 0.4f, hipY);
    const float footY = bottom;
    const ColorRGB legColor = main.Scaled(0.75f);
    Limb(hip, Vec2(cx + legSwing * 0.8f + lean * 0.2f, footY), h * 0.052f, legColor, alpha);
    Limb(hip, Vec2(cx - legSwing * 0.8f + lean * 0.2f, footY), h * 0.052f, legColor, alpha);

    // --- 胴 -----------------------------------------------------------------
    const Rect torso(cx - torsoW * 0.5f + lean * 0.5f, shoulderY,
                     cx + torsoW * 0.5f + lean * 0.5f, hipY + h * 0.03f);
    draw::GradientRectV(torso, main.Scaled(1.2f), main.Scaled(0.8f), alpha, 8);
    draw::StrokeRect(torso, accent.Scaled(0.8f), 2.0f, alpha);
    // 胸のライン
    draw::Line(torso.left + 3.0f, torso.top + torso.Height() * 0.35f,
               torso.right - 3.0f, torso.top + torso.Height() * 0.45f, art.trim, 2.0f, alpha);

    // --- マント（騎士のみ） --------------------------------------------------
    if (art.style == ArtStyle::Knight) {
        const float capeX = cx - dir * torsoW * 0.55f + lean * 0.3f;
        const Vec2 a(capeX, shoulderY - h * 0.02f);
        const Vec2 b(capeX - dir * w * 0.45f, hipY + h * 0.22f);
        const Vec2 c(capeX + dir * torsoW * 0.2f, hipY + h * 0.10f);
        draw::Triangle(a, b, c, art.trim.Scaled(0.55f), true, alpha);
    }

    // --- 頭 -----------------------------------------------------------------
    const float headX = cx + lean * 0.9f + dir * w * 0.04f;
    draw::Circle(headX, headY, headR, accent, true, 1.0f, alpha);
    draw::Circle(headX, headY, headR, main.Scaled(0.5f), false, 2.0f, alpha);
    // 視線方向
    draw::Circle(headX + dir * headR * 0.42f, headY - headR * 0.1f, headR * 0.17f,
                 art.trim, true, 1.0f, alpha);

    // --- 盾 -----------------------------------------------------------------
    if (art.hasShield) {
        const float shieldX = cx - dir * (torsoW * 0.55f) + lean * 0.5f
                            + ((pose == PoseKind::Guard) ? dir * torsoW * 1.5f : 0.0f);
        const float shieldY = shoulderY + h * 0.10f;
        const Rect shield = Rect::FromCenter(shieldX, shieldY, w * 0.30f, h * 0.26f);
        draw::FillRect(shield, main.Scaled(1.35f), alpha);
        draw::StrokeRect(shield, art.trim, 2.0f, alpha);
    }

    // --- 腕と武器 -----------------------------------------------------------
    const Vec2 shoulder(cx + dir * torsoW * 0.35f + lean * 0.7f, shoulderY + h * 0.05f);
    const float armLen = h * 0.20f;
    const float angle = ToWeaponAngle(swing.angleDeg, facing);
    const float armTwist = math::DegToRad(dir > 0.0f ? 20.0f : -20.0f);
    // 突き技は肩から手までまとめて前へ送り出す
    const Vec2 handOffset(dir * h * swing.reachOut, -h * swing.liftUp);

    // 腕は武器角度に追従（下向きを 90 度とする座標系に合わせる）
    Vec2 hand = Rotate(shoulder, armLen, angle - armTwist);
    hand.x += handOffset.x;
    hand.y += handOffset.y;
    Limb(shoulder, hand, h * 0.045f, main.Scaled(1.1f), alpha);

    if (art.hasWeapon) {
        const float weaponLen = h * 0.46f * WeaponReachScale(art.weapon);

        // 振っている最中は少し前のコマを薄く重ねて軌跡にする
        if (swinging) {
            for (int i = 1; i <= 2; ++i) {
                const float back = 0.035f * static_cast<float>(i);
                if (phase - back <= 0.0f) break;
                const WeaponSwing past = SampleSwing(pose, phase - back, art, variant);
                const float pastAngle = ToWeaponAngle(past.angleDeg, facing);
                Vec2 pastHand = Rotate(shoulder, armLen, pastAngle - armTwist);
                pastHand.x += dir * h * past.reachOut;
                pastHand.y -= h * past.liftUp;
                const int trail = math::ClampInt(alpha / (2 + i * 2), 0, 255);
                DrawWeapon(pastHand, pastAngle, facing, weaponLen, art.weapon,
                           art.trim, art.trim, trail);
            }
        }

        DrawWeapon(hand, angle, facing, weaponLen, art.weapon, accent, art.trim, alpha);

        // スキル発動中は軌跡を光らせる
        if (pose == PoseKind::Skill) {
            const Vec2 tip = Rotate(hand, weaponLen, angle);
            draw::Glow(tip.x, tip.y, h * 0.16f, art.trim, 150, 4);
        }

        // --- 二刀流：逆手にもう一振り --------------------------------------
        if (art.hasOffHandWeapon) {
            const Vec2 offShoulder(cx - dir * torsoW * 0.35f + lean * 0.5f, shoulderY + h * 0.06f);
            // 主武器とずらして振る（ずらし方はモーションごとに決まっている）
            const float offAngle = angle + math::DegToRad(dir > 0.0f ? swing.offHandDeg
                                                                    : -swing.offHandDeg);
            Vec2 offHand = Rotate(offShoulder, armLen * 0.95f, offAngle - armTwist);
            offHand.x += handOffset.x * 0.8f;
            offHand.y += handOffset.y * 0.8f;
            Limb(offShoulder, offHand, h * 0.042f, main, alpha);
            DrawWeapon(offHand, offAngle, facing, weaponLen * 0.95f, art.weapon,
                       accent.Scaled(0.9f), art.trim, alpha);

            if (pose == PoseKind::Skill) {
                const Vec2 tip = Rotate(offHand, weaponLen * 0.95f, offAngle);
                draw::Glow(tip.x, tip.y, h * 0.14f, art.trim, 130, 4);
            }
        }
    }
}

//------------------------------------------------------------------------------
// 四足獣
//------------------------------------------------------------------------------
void DrawBeast(const Rect& rect, int facing, PoseKind pose, float phase,
               const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent)
{
    const float w = rect.Width();
    const float h = rect.Height();
    const float cx = rect.CenterX();
    const float bottom = rect.bottom;
    const float dir = (facing >= 0) ? 1.0f : -1.0f;

    if (pose == PoseKind::Dead) {
        const Rect body = Rect::FromFoot(cx, bottom, w * 0.55f, h * 0.30f);
        draw::FillRect(body, main.Scaled(0.7f), alpha);
        return;
    }

    float legSwing = 0.0f;
    float lunge = 0.0f;
    if (pose == PoseKind::Walk || pose == PoseKind::Run) {
        legSwing = std::sin(phase * kTwoPi) * h * 0.16f;
    } else if (pose == PoseKind::Attack || pose == PoseKind::Skill) {
        lunge = dir * w * 0.18f * std::sin(math::Clamp(phase, 0.0f, 1.0f) * math::kPi);
    } else if (pose == PoseKind::Idle) {
        lunge = std::sin(phase * kTwoPi) * h * 0.01f;
    }

    const float bodyTop = bottom - h * 0.78f;
    const float bodyBottom = bottom - h * 0.30f;

    // 脚
    const ColorRGB legColor = main.Scaled(0.7f);
    for (int i = 0; i < 4; ++i) {
        const float t = (i < 2) ? 0.30f : 0.70f;
        const float baseX = rect.left + w * t + (i % 2 == 0 ? -w * 0.05f : w * 0.05f);
        const float swing = ((i % 2 == 0) ? legSwing : -legSwing) * ((i < 2) ? 1.0f : -1.0f);
        Limb(Vec2(baseX + lunge, bodyBottom), Vec2(baseX + swing + lunge, bottom), h * 0.06f, legColor, alpha);
    }

    // 胴
    const Rect body(rect.left + w * 0.16f + lunge, bodyTop, rect.right - w * 0.16f + lunge, bodyBottom);
    draw::GradientRectV(body, main.Scaled(1.2f), main.Scaled(0.75f), alpha, 8);
    draw::StrokeRect(body, accent.Scaled(0.6f), 2.0f, alpha);

    // 背中のトゲ
    for (int i = 0; i < 4; ++i) {
        const float sx = body.left + body.Width() * (0.2f + 0.2f * static_cast<float>(i));
        draw::Triangle(Vec2(sx - w * 0.04f, body.top), Vec2(sx + w * 0.04f, body.top),
                       Vec2(sx, body.top - h * 0.12f), art.trim, true, alpha);
    }

    // 頭
    const float headX = cx + dir * w * 0.42f + lunge;
    const float headY = bodyTop + h * 0.06f;
    const float headR = h * 0.15f;
    draw::Circle(headX, headY, headR, main.Scaled(1.15f), true, 1.0f, alpha);
    // 鼻先
    draw::Triangle(Vec2(headX, headY - headR * 0.5f), Vec2(headX, headY + headR * 0.6f),
                   Vec2(headX + dir * headR * 1.5f, headY + headR * 0.15f), main.Scaled(1.05f), true, alpha);
    // 耳
    draw::Triangle(Vec2(headX - dir * headR * 0.3f, headY - headR * 0.7f),
                   Vec2(headX + dir * headR * 0.3f, headY - headR * 0.7f),
                   Vec2(headX, headY - headR * 1.7f), main.Scaled(0.9f), true, alpha);
    // 目（攻撃時は発光）
    const ColorRGB eye = (pose == PoseKind::Attack || pose == PoseKind::Skill) ? art.trim : accent;
    draw::Circle(headX + dir * headR * 0.45f, headY - headR * 0.1f, headR * 0.20f, eye, true, 1.0f, alpha);
    if (pose == PoseKind::Skill) draw::Glow(headX + dir * headR * 0.5f, headY, headR * 0.7f, art.trim, 120, 3);

    // 尻尾
    const float tailX = cx - dir * w * 0.46f + lunge;
    const float tailWave = std::sin(phase * kTwoPi) * h * 0.08f;
    draw::Line(tailX, bodyTop + h * 0.12f, tailX - dir * w * 0.18f, bodyTop - h * 0.05f + tailWave,
               main.Scaled(0.85f), h * 0.05f, alpha);
}

//------------------------------------------------------------------------------
// 岩石系
//------------------------------------------------------------------------------
void DrawGolem(const Rect& rect, int facing, PoseKind pose, float phase,
               const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent)
{
    const float w = rect.Width();
    const float h = rect.Height();
    const float cx = rect.CenterX();
    const float bottom = rect.bottom;
    const float dir = (facing >= 0) ? 1.0f : -1.0f;

    if (pose == PoseKind::Dead) {
        // 崩れた瓦礫
        for (int i = 0; i < 5; ++i) {
            const float bx = cx + (static_cast<float>(i) - 2.0f) * w * 0.22f;
            const float bh = h * (0.10f + 0.04f * static_cast<float>((i * 7) % 3));
            draw::FillRect(Rect::FromFoot(bx, bottom, w * 0.12f, bh), main.Scaled(0.65f), alpha);
        }
        return;
    }

    float bob = 0.0f;
    float lunge = 0.0f;
    if (pose == PoseKind::Walk || pose == PoseKind::Run) {
        bob = std::fabs(std::sin(phase * kTwoPi)) * h * 0.04f;
    } else if (pose == PoseKind::Attack || pose == PoseKind::Skill) {
        lunge = dir * w * 0.2f * std::sin(math::Clamp(phase, 0.0f, 1.0f) * math::kPi);
    } else if (pose == PoseKind::Idle) {
        bob = std::sin(phase * kTwoPi) * h * 0.012f;
    }

    // 脚
    const ColorRGB rock = main.Scaled(0.8f);
    draw::FillRect(Rect::FromFoot(cx - w * 0.22f, bottom, w * 0.14f, h * 0.30f), rock, alpha);
    draw::FillRect(Rect::FromFoot(cx + w * 0.22f, bottom, w * 0.14f, h * 0.30f), rock, alpha);

    // 胴（複数の岩塊）
    const Rect torso = Rect::FromFoot(cx + lunge * 0.3f, bottom - h * 0.26f + bob, w * 0.40f, h * 0.44f);
    draw::GradientRectV(torso, main.Scaled(1.25f), main.Scaled(0.8f), alpha, 8);
    draw::StrokeRect(torso, accent.Scaled(0.5f), 3.0f, alpha);

    // コア（発光）
    const float coreY = torso.CenterY();
    const ColorRGB core = (pose == PoseKind::Skill || pose == PoseKind::Attack)
                        ? art.trim : art.trim.Scaled(0.7f);
    draw::Glow(cx + lunge * 0.3f, coreY, h * 0.09f, core, 170, 4);
    draw::Circle(cx + lunge * 0.3f, coreY, h * 0.045f, palette::kWhite, true, 1.0f, alpha);

    // 頭
    const Rect head = Rect::FromCenter(cx + dir * w * 0.06f + lunge * 0.4f, torso.top - h * 0.10f,
                                       w * 0.28f, h * 0.20f);
    draw::FillRect(head, main.Scaled(1.1f), alpha);
    draw::StrokeRect(head, accent.Scaled(0.5f), 2.0f, alpha);
    draw::Circle(head.CenterX() + dir * head.Width() * 0.22f, head.CenterY(), h * 0.025f, core, true, 1.0f, alpha);

    // 腕（攻撃時は前方へ）
    const float armSwing = (pose == PoseKind::Attack || pose == PoseKind::Skill)
                         ? math::Clamp(phase, 0.0f, 1.0f) : 0.0f;
    const Vec2 shoulder(cx + dir * w * 0.34f + lunge * 0.5f, torso.top + h * 0.06f);
    const Vec2 hand(shoulder.x + dir * w * (0.10f + armSwing * 0.45f),
                    shoulder.y + h * (0.24f - armSwing * 0.22f));
    Limb(shoulder, hand, h * 0.075f, main, alpha);
    draw::FillRect(Rect::FromCenter(hand.x, hand.y, w * 0.24f, h * 0.14f), main.Scaled(1.2f), alpha);
}

//------------------------------------------------------------------------------
// 浮遊体
//------------------------------------------------------------------------------
void DrawWisp(const Rect& rect, int facing, PoseKind pose, float phase,
              const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent)
{
    const float w = rect.Width();
    const float h = rect.Height();
    const float cx = rect.CenterX();
    const float cy = rect.CenterY() + std::sin(phase * kTwoPi) * h * 0.06f;
    const float dir = (facing >= 0) ? 1.0f : -1.0f;

    if (pose == PoseKind::Dead) {
        draw::Circle(cx, cy, w * 0.2f, main, true, 1.0f, math::ClampInt(alpha / 2, 0, 255));
        return;
    }

    draw::Glow(cx, cy, w * 0.55f, art.trim, 130, 5);
    draw::Circle(cx, cy, w * 0.30f, main, true, 1.0f, alpha);
    draw::Circle(cx, cy, w * 0.30f, accent, false, 2.0f, alpha);
    draw::Circle(cx + dir * w * 0.10f, cy - h * 0.04f, w * 0.08f, palette::kWhite, true, 1.0f, alpha);

    // 周回する粒
    for (int i = 0; i < 3; ++i) {
        const float a = phase * kTwoPi + static_cast<float>(i) * (kTwoPi / 3.0f);
        draw::Circle(cx + std::cos(a) * w * 0.45f, cy + std::sin(a) * h * 0.22f,
                     w * 0.05f, art.trim, true, 1.0f, alpha);
    }
}

} // namespace

//------------------------------------------------------------------------------
// 武器
//   刀身は「光の当たる側 / 影側」の 2 色に塗り分け、暗い縁取りを付けて立体感を出す。
//   残像（alpha が低い）は細部を省いたシルエットだけを描く。
//------------------------------------------------------------------------------
namespace {

// 武器の軸に沿った座標系（along : 手元から先端へ / side : 軸に直交）
struct WeaponFrame
{
    Vec2 hand;
    Vec2 dir;
    Vec2 perp;
    int  alpha = 255;

    Vec2 P(float along, float side) const
    {
        return Vec2(hand.x + dir.x * along + perp.x * side, hand.y + dir.y * along + perp.y * side);
    }
    void Seg(float a1, float s1, float a2, float s2, const ColorRGB& color, float thickness,
             int a = -1) const
    {
        const Vec2 p1 = P(a1, s1);
        const Vec2 p2 = P(a2, s2);
        draw::Line(p1.x, p1.y, p2.x, p2.y, color, thickness, a < 0 ? alpha : a);
    }
    void Tri(const Vec2& p1, const Vec2& p2, const Vec2& p3, const ColorRGB& color, int a = -1) const
    {
        draw::Triangle(p1, p2, p3, color, true, a < 0 ? alpha : a);
    }
    void Quad(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec2& p4, const ColorRGB& color) const
    {
        Tri(p1, p2, p3, color);
        Tri(p1, p3, p4, color);
    }
    void Dot(float along, float side, float radius, const ColorRGB& color, bool fill = true,
             float thickness = 1.0f, int a = -1) const
    {
        const Vec2 c = P(along, side);
        draw::Circle(c.x, c.y, radius, color, fill, thickness, a < 0 ? alpha : a);
    }

    // 左右対称の刀身（stations は手元から先端へ (along, 半幅) の並び。最後は先端）
    //   上半分を明るく、下半分を暗く塗り、外側に縁取りを付ける
    void Blade(const std::vector<std::pair<float, float>>& stations, const ColorRGB& light,
               const ColorRGB& dark, const ColorRGB& outline, float outlineWidth) const
    {
        const size_t n = stations.size();
        if (n < 2) return;
        // 縁取り：少し太らせた形を暗い色で先に塗る
        for (size_t i = 0; i + 1 < n; ++i) {
            const float a1 = stations[i].first - (i == 0 ? outlineWidth : 0.0f);
            const float a2 = stations[i + 1].first + (i + 2 == n ? outlineWidth * 1.6f : 0.0f);
            const float w1 = stations[i].second + outlineWidth;
            const float w2 = stations[i + 1].second + (i + 2 == n ? 0.0f : outlineWidth);
            Quad(P(a1, -w1), P(a2, -w2), P(a2, w2), P(a1, w1), outline);
        }
        // 下地：2 色の境目や三角形の継ぎ目に縁取りの色が透けないよう、中間色で一度塗る
        const ColorRGB mid = ColorRGB::Lerp(light, dark, 0.5f);
        for (size_t i = 0; i + 1 < n; ++i) {
            Quad(P(stations[i].first, -stations[i].second), P(stations[i + 1].first, -stations[i + 1].second),
                 P(stations[i + 1].first, stations[i + 1].second), P(stations[i].first, stations[i].second), mid);
        }
        // 本体：光の当たる側（side < 0）と影側
        for (size_t i = 0; i + 1 < n; ++i) {
            const float a1 = stations[i].first;
            const float a2 = stations[i + 1].first;
            const float w1 = stations[i].second;
            const float w2 = stations[i + 1].second;
            Quad(P(a1, -w1), P(a2, -w2), P(a2, 0.0f), P(a1, 0.0f), light);
            Quad(P(a1, 0.0f), P(a2, 0.0f), P(a2, w2), P(a1, w1), dark);
        }
    }

    // 革を巻いた柄（from → to）。巻き目を斜めの線で入れる
    void Grip(float from, float to, float width, const ColorRGB& leather, const ColorRGB& outline,
              int wraps) const
    {
        Seg(from, 0.0f, to, 0.0f, outline, width + 3.0f);
        Seg(from, 0.0f, to, 0.0f, leather, width);
        Seg(from, -width * 0.18f, to, -width * 0.18f, leather.Scaled(1.35f), width * 0.22f);
        for (int i = 0; i < wraps; ++i) {
            const float t = (static_cast<float>(i) + 0.5f) / static_cast<float>(wraps);
            const float a = from + (to - from) * t;
            const float step = (to - from) / static_cast<float>(wraps) * 0.35f;
            Seg(a - step, -width * 0.48f, a + step, width * 0.48f, outline, math::MaxF(1.0f, width * 0.14f));
        }
    }

    // 柄頭（丸い金具＋小さな宝石）
    void Pommel(float along, float radius, const ColorRGB& fitting, const ColorRGB& gem,
                const ColorRGB& outline) const
    {
        Dot(along, 0.0f, radius + 1.5f, outline);
        Dot(along, 0.0f, radius, fitting);
        Dot(along, -radius * 0.35f, radius * 0.45f, fitting.Scaled(1.3f));
        Dot(along, 0.0f, radius * 0.38f, gem);
    }
};

ColorRGB Leather() { return ColorRGB(96, 64, 44); }

// 残像用：細部を省いたシルエット
void DrawWeaponSilhouette(const WeaponFrame& f, float length, WeaponType type, const ColorRGB& color)
{
    switch (type) {
    case WeaponType::OneHandSword:
        f.Tri(f.P(length * 0.05f, -length * 0.075f), f.P(length * 0.05f, length * 0.075f),
              f.P(length, 0.0f), color);
        break;
    case WeaponType::OneHandMace:
        f.Seg(0.0f, 0.0f, length * 0.7f, 0.0f, color, length * 0.06f);
        f.Dot(length * 0.8f, 0.0f, length * 0.16f, color);
        break;
    case WeaponType::Dagger: {
        const float len = length * 0.55f;
        f.Tri(f.P(0.0f, -len * 0.13f), f.P(0.0f, len * 0.13f), f.P(len, 0.0f), color);
        break;
    }
    case WeaponType::Rapier: {
        const float len = length * 1.12f;
        f.Seg(0.0f, 0.0f, len, 0.0f, color, len * 0.035f);
        break;
    }
    case WeaponType::Spear: {
        const float len = length * 1.30f;
        f.Seg(len * 0.4f, 0.0f, len * 0.8f, 0.0f, color, len * 0.03f);
        f.Tri(f.P(len * 0.78f, -len * 0.06f), f.P(len * 0.78f, len * 0.06f), f.P(len, 0.0f), color);
        break;
    }
    default:
        break;
    }
}

} // namespace

void DrawWeapon(const Vec2& handPos, float angleRad, int facing, float length,
                WeaponType type, const ColorRGB& metal, const ColorRGB& accent, int alpha)
{
    // 向きは angleRad に含まれているため facing は形状調整用にのみ使う
    (void)facing;
    WeaponFrame f;
    f.hand = handPos;
    f.dir = Vec2(std::cos(angleRad), std::sin(angleRad));
    f.perp = Vec2(-f.dir.y, f.dir.x);
    f.alpha = alpha;

    // 残像は形だけ
    if (alpha < 200) {
        DrawWeaponSilhouette(f, length, type, metal);
        return;
    }

    const ColorRGB light = ColorRGB::Lerp(metal, palette::kWhite, 0.25f);   // 光の当たる面
    const ColorRGB dark = ColorRGB::Lerp(metal.Scaled(0.58f), accent, 0.12f); // 影の面
    const ColorRGB outline(16, 20, 30);
    // 鍔や柄頭などの金具（刀身より少し暗く、アクセント色を差す）
    const ColorRGB fitting = ColorRGB::Lerp(metal.Scaled(0.72f), accent, 0.30f);
    const ColorRGB edge = palette::kWhite;
    const float ol = math::MaxF(1.5f, length * 0.014f);

    switch (type) {
    //--------------------------------------------------------------------------
    // 片手剣：両刃の直剣。樋（フラー）・十字鍔・宝石つきの柄頭
    //--------------------------------------------------------------------------
    case WeaponType::OneHandSword: {
        const float L = length;
        const float bw = L * 0.072f;
        f.Grip(-L * 0.17f, -L * 0.01f, L * 0.06f, Leather(), outline, 4);
        f.Pommel(-L * 0.19f, L * 0.04f, fitting, accent, outline);

        f.Blade({ { L * 0.03f, bw * 0.95f }, { L * 0.10f, bw }, { L * 0.80f, bw * 0.92f },
                  { L * 1.00f, 0.0f } },
                light, dark, outline, ol);
        // 樋（中央の溝）と刃先の光
        f.Seg(L * 0.09f, 0.0f, L * 0.68f, 0.0f, dark.Scaled(0.7f), L * 0.022f);
        f.Seg(L * 0.09f, -L * 0.011f, L * 0.68f, -L * 0.011f, light, math::MaxF(1.0f, L * 0.006f));
        f.Seg(L * 0.05f, -bw * 0.80f, L * 0.86f, -bw * 0.62f, edge, math::MaxF(1.0f, L * 0.008f),
              math::ClampInt(alpha - 90, 0, 255));

        // 十字鍔（中央が太く両端が丸い）
        f.Seg(L * 0.015f, -L * 0.20f, L * 0.015f, L * 0.20f, outline, L * 0.058f);
        f.Seg(L * 0.015f, -L * 0.19f, L * 0.015f, L * 0.19f, fitting, L * 0.040f);
        f.Seg(L * 0.010f, -L * 0.18f, L * 0.010f, L * 0.18f, fitting.Scaled(1.3f), L * 0.010f);
        for (float side : { -L * 0.20f, L * 0.20f }) {
            f.Dot(L * 0.015f, side, L * 0.030f, outline);
            f.Dot(L * 0.015f, side, L * 0.022f, fitting);
        }
        f.Dot(L * 0.015f, 0.0f, L * 0.034f, outline);
        f.Dot(L * 0.015f, 0.0f, L * 0.026f, accent);
        f.Dot(L * 0.010f, -L * 0.008f, L * 0.009f, edge);
        break;
    }
    //--------------------------------------------------------------------------
    // 片手棍：金属の柄と、6 枚の出縁（フランジ）を持つ頭部
    //--------------------------------------------------------------------------
    case WeaponType::OneHandMace: {
        const float L = length;
        f.Grip(-L * 0.12f, L * 0.14f, L * 0.064f, Leather(), outline, 4);
        f.Pommel(-L * 0.15f, L * 0.038f, fitting, accent, outline);
        // 柄（金属）
        f.Seg(L * 0.14f, 0.0f, L * 0.70f, 0.0f, outline, L * 0.062f);
        f.Seg(L * 0.14f, 0.0f, L * 0.70f, 0.0f, dark, L * 0.044f);
        f.Seg(L * 0.14f, -L * 0.010f, L * 0.70f, -L * 0.010f, light, L * 0.012f);
        // 口金
        for (float a : { L * 0.15f, L * 0.64f }) {
            f.Seg(a, -L * 0.045f, a, L * 0.045f, outline, L * 0.042f);
            f.Seg(a, -L * 0.040f, a, L * 0.040f, fitting, L * 0.028f);
        }

        // 頭部：出縁を放射状に並べる（上側は明るく、下側は暗く）
        const Vec2 head = f.P(L * 0.82f, 0.0f);
        const float baseAngle = std::atan2(f.dir.y, f.dir.x);
        for (int pass = 0; pass < 2; ++pass) {
            for (int k = 0; k < 6; ++k) {
                const float a = baseAngle + kTwoPi * static_cast<float>(k) / 6.0f + 0.26f;
                const Vec2 out(std::cos(a), std::sin(a));
                const Vec2 side(-out.y, out.x);
                const float grow = (pass == 0) ? ol : 0.0f;
                const float r0 = L * 0.07f;
                const float r1 = L * 0.20f + grow * 1.4f;
                const float w = L * 0.055f + grow;
                const Vec2 b1(head.x + out.x * r0 + side.x * w, head.y + out.y * r0 + side.y * w);
                const Vec2 b2(head.x + out.x * r0 - side.x * w, head.y + out.y * r0 - side.y * w);
                const Vec2 tip(head.x + out.x * r1, head.y + out.y * r1);
                // 光の向き（武器の軸に対して side < 0 側）を向いた出縁ほど明るい
                const float facingLight = -(out.x * f.perp.x + out.y * f.perp.y);
                const ColorRGB color = (pass == 0) ? outline
                                     : ColorRGB::Lerp(dark, light, 0.5f + 0.5f * facingLight);
                f.Tri(b1, b2, tip, color);
            }
        }
        f.Dot(L * 0.82f, 0.0f, L * 0.095f + ol, outline);
        f.Dot(L * 0.82f, 0.0f, L * 0.095f, dark);
        f.Dot(L * 0.80f, -L * 0.030f, L * 0.050f, light);
        f.Dot(L * 0.82f, 0.0f, L * 0.030f, accent);
        // 先端の短い突起
        f.Tri(f.P(L * 0.90f, -L * 0.034f - ol), f.P(L * 0.90f, L * 0.034f + ol), f.P(L * 1.05f + ol, 0.0f),
              outline);
        f.Tri(f.P(L * 0.90f, -L * 0.030f), f.P(L * 0.90f, 0.0f), f.P(L * 1.04f, 0.0f), light);
        f.Tri(f.P(L * 0.90f, 0.0f), f.P(L * 0.90f, L * 0.030f), f.P(L * 1.04f, 0.0f), dark);
        break;
    }
    //--------------------------------------------------------------------------
    // 短剣：木の葉形の刀身と、先が反った小さな鍔
    //--------------------------------------------------------------------------
    case WeaponType::Dagger: {
        const float len = length * 0.55f;
        f.Grip(-len * 0.24f, -len * 0.01f, len * 0.10f, Leather(), outline, 3);
        f.Pommel(-len * 0.27f, len * 0.065f, fitting, accent, outline);

        f.Blade({ { len * 0.03f, len * 0.085f }, { len * 0.12f, len * 0.11f }, { len * 0.42f, len * 0.125f },
                  { len * 0.80f, len * 0.07f }, { len * 1.00f, 0.0f } },
                light, dark, outline, ol);
        // 鎬（中央の稜線）
        f.Seg(len * 0.06f, 0.0f, len * 0.94f, 0.0f, dark.Scaled(0.75f), math::MaxF(1.0f, len * 0.016f));
        f.Seg(len * 0.06f, -len * 0.09f, len * 0.80f, -len * 0.06f, edge, math::MaxF(1.0f, len * 0.012f),
              math::ClampInt(alpha - 90, 0, 255));

        // 鍔：両端を刀身側へ反らせる
        f.Seg(len * 0.01f, -len * 0.20f, len * 0.01f, len * 0.20f, outline, len * 0.085f);
        f.Seg(len * 0.01f, -len * 0.19f, len * 0.01f, len * 0.19f, fitting, len * 0.060f);
        for (float s : { -1.0f, 1.0f }) {
            f.Seg(len * 0.01f, s * len * 0.19f, len * 0.09f, s * len * 0.26f, outline, len * 0.060f);
            f.Seg(len * 0.01f, s * len * 0.19f, len * 0.09f, s * len * 0.26f, fitting, len * 0.038f);
        }
        f.Dot(len * 0.01f, 0.0f, len * 0.045f, accent);
        break;
    }
    //--------------------------------------------------------------------------
    // 細剣：細く長い刀身。碗状の護拳・十字の鍔・護拳から柄頭へ回る弓
    //--------------------------------------------------------------------------
    case WeaponType::Rapier: {
        const float len = length * 1.12f;
        f.Grip(-len * 0.15f, -len * 0.005f, len * 0.040f, Leather(), outline, 5);
        f.Pommel(-len * 0.175f, len * 0.034f, fitting, accent, outline);

        f.Blade({ { len * 0.05f, len * 0.020f }, { len * 0.12f, len * 0.017f }, { len * 0.92f, len * 0.008f },
                  { len * 1.00f, 0.0f } },
                light, dark, outline, math::MaxF(1.2f, ol * 0.8f));
        f.Seg(len * 0.07f, -len * 0.010f, len * 0.92f, -len * 0.005f, edge, math::MaxF(1.0f, len * 0.004f),
              math::ClampInt(alpha - 80, 0, 255));

        // 護拳の弓（鍔から柄頭へ、手の甲側を回る曲線）
        Vec2 prev = f.P(len * 0.02f, len * 0.11f);
        for (int i = 1; i <= 8; ++i) {
            const float t = static_cast<float>(i) / 8.0f;
            const float u = 1.0f - t;
            // 2 次ベジェ：(0.02, 0.11) → (-0.07, 0.17) → (-0.17, 0.035)
            const float a = u * u * 0.02f + 2.0f * u * t * (-0.07f) + t * t * (-0.17f);
            const float sd = u * u * 0.11f + 2.0f * u * t * 0.17f + t * t * 0.035f;
            const Vec2 cur = f.P(len * a, len * sd);
            draw::Line(prev.x, prev.y, cur.x, cur.y, outline, len * 0.024f, alpha);
            draw::Circle(cur.x, cur.y, len * 0.012f, outline, true, 1.0f, alpha);
            prev = cur;
        }
        // 縁取りを全部引いてから中身を重ねる（継ぎ目に隙間が出ないよう関節に丸を置く）
        prev = f.P(len * 0.02f, len * 0.11f);
        for (int i = 1; i <= 8; ++i) {
            const float t = static_cast<float>(i) / 8.0f;
            const float u = 1.0f - t;
            const float a = u * u * 0.02f + 2.0f * u * t * (-0.07f) + t * t * (-0.17f);
            const float sd = u * u * 0.11f + 2.0f * u * t * 0.17f + t * t * 0.035f;
            const Vec2 cur = f.P(len * a, len * sd);
            draw::Line(prev.x, prev.y, cur.x, cur.y, fitting, len * 0.013f, alpha);
            draw::Circle(cur.x, cur.y, len * 0.0065f, fitting, true, 1.0f, alpha);
            prev = cur;
        }
        // 十字の鍔
        f.Seg(len * 0.03f, -len * 0.13f, len * 0.03f, len * 0.12f, outline, len * 0.026f);
        f.Seg(len * 0.03f, -len * 0.125f, len * 0.03f, len * 0.115f, fitting, len * 0.015f);
        f.Dot(len * 0.03f, -len * 0.13f, len * 0.018f, fitting);
        // 碗状の護拳
        f.Dot(len * 0.035f, 0.0f, len * 0.070f + ol, outline);
        f.Dot(len * 0.035f, 0.0f, len * 0.070f, dark);
        f.Dot(len * 0.025f, -len * 0.020f, len * 0.042f, light);
        f.Dot(len * 0.035f, 0.0f, len * 0.070f, accent, false, math::MaxF(1.5f, len * 0.010f));
        break;
    }
    //--------------------------------------------------------------------------
    // 槍：木目の柄・布を巻いた握り・房飾り・稜線のある木の葉形の穂先
    //--------------------------------------------------------------------------
    case WeaponType::Spear: {
        const float len = length * 1.30f;
        const ColorRGB wood(126, 90, 58);
        // 柄
        f.Seg(-len * 0.30f, 0.0f, len * 0.78f, 0.0f, outline, len * 0.042f);
        f.Seg(-len * 0.30f, 0.0f, len * 0.78f, 0.0f, wood, len * 0.030f);
        f.Seg(-len * 0.30f, -len * 0.007f, len * 0.78f, -len * 0.007f, wood.Scaled(1.35f), len * 0.007f);
        f.Seg(-len * 0.20f, len * 0.006f, len * 0.60f, len * 0.006f, wood.Scaled(0.7f), len * 0.004f);
        // 石突
        f.Seg(-len * 0.33f, 0.0f, -len * 0.27f, 0.0f, outline, len * 0.050f);
        f.Seg(-len * 0.33f, 0.0f, -len * 0.27f, 0.0f, fitting, len * 0.036f);
        // 握りの布巻き
        for (int i = 0; i < 4; ++i) {
            const float a = -len * 0.07f + len * 0.045f * static_cast<float>(i);
            f.Seg(a - len * 0.012f, -len * 0.020f, a + len * 0.012f, len * 0.020f, outline, len * 0.020f);
            f.Seg(a - len * 0.012f, -len * 0.018f, a + len * 0.012f, len * 0.018f, accent.Scaled(0.75f),
                  len * 0.012f);
        }
        // 房飾り（口金から垂れる）
        for (float s : { 0.0f, 0.03f }) {
            f.Seg(len * 0.76f, len * 0.015f, len * (0.67f + s), len * (0.085f + s * 0.6f), accent,
                  math::MaxF(1.5f, len * 0.010f));
        }
        // 口金
        f.Quad(f.P(len * 0.73f, -len * 0.028f), f.P(len * 0.81f, -len * 0.022f),
               f.P(len * 0.81f, len * 0.022f), f.P(len * 0.73f, len * 0.028f), outline);
        f.Quad(f.P(len * 0.735f, -len * 0.022f), f.P(len * 0.805f, -len * 0.017f),
               f.P(len * 0.805f, len * 0.017f), f.P(len * 0.735f, len * 0.022f), fitting);
        f.Seg(len * 0.745f, -len * 0.030f, len * 0.745f, len * 0.030f, accent, len * 0.012f);
        // 穂先
        f.Blade({ { len * 0.80f, len * 0.030f }, { len * 0.86f, len * 0.058f }, { len * 0.93f, len * 0.040f },
                  { len * 1.00f, 0.0f } },
                light, dark, outline, ol);
        f.Seg(len * 0.81f, 0.0f, len * 0.985f, 0.0f, dark.Scaled(0.7f), math::MaxF(1.0f, len * 0.006f));
        f.Seg(len * 0.82f, -len * 0.035f, len * 0.95f, -len * 0.025f, edge, math::MaxF(1.0f, len * 0.004f),
              math::ClampInt(alpha - 90, 0, 255));
        break;
    }
    default:
        break;
    }
}

void DrawActorShadow(float centerX, float groundY, float width, int alpha)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, math::ClampInt(alpha, 0, 255));
    DrawOvalAA(centerX, groundY, width * 0.5f, width * 0.16f, 20, draw::ToDx(ColorRGB(0, 0, 0)), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void DrawActor(const Rect& screenRect, int facing, PoseKind pose, float phase,
               const ActorArt& art, const Animator* animator, int alpha, float flash,
               int motionVariant)
{
    // --- スプライトがあればそちらを使う ---------------------------------------
    if (animator != nullptr && animator->HasArt()) {
        const AnimationClip* clip = animator->Clip();
        const TextureAsset* asset = clip->asset;
        const int handle = asset->Frame(animator->FrameIndex());
        if (handle >= 0) {
            const float rate = (asset->frameHeight > 0)
                             ? screenRect.Height() / static_cast<float>(asset->frameHeight)
                             : 1.0f;
            const int cx = static_cast<int>(screenRect.CenterX());
            const int cy = static_cast<int>(screenRect.bottom - screenRect.Height() * 0.5f);

            if (alpha < 255) SetDrawBlendMode(DX_BLENDMODE_ALPHA, math::ClampInt(alpha, 0, 255));
            DrawRotaGraph2(cx, cy, asset->frameWidth / 2, asset->frameHeight / 2,
                           static_cast<double>(rate), 0.0, handle, TRUE, facing < 0 ? TRUE : FALSE);
            if (alpha < 255) SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 被弾フラッシュ
            if (flash > 0.01f) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(static_cast<int>(flash * 255.0f), 0, 255));
                DrawRotaGraph2(cx, cy, asset->frameWidth / 2, asset->frameHeight / 2,
                               static_cast<double>(rate), 0.0, handle, TRUE, facing < 0 ? TRUE : FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            return;
        }
    }

    // --- 代替表示 ------------------------------------------------------------
    ColorRGB main = art.main;
    ColorRGB accent = art.accent;
    if (flash > 0.01f) {
        main = ColorRGB::Lerp(main, palette::kWhite, math::Clamp(flash, 0.0f, 1.0f));
        accent = ColorRGB::Lerp(accent, palette::kWhite, math::Clamp(flash, 0.0f, 1.0f));
    }

    switch (art.style) {
    case ArtStyle::Beast:
        DrawBeast(screenRect, facing, pose, phase, art, alpha, main, accent);
        break;
    case ArtStyle::Golem:
        DrawGolem(screenRect, facing, pose, phase, art, alpha, main, accent);
        break;
    case ArtStyle::Wisp:
        DrawWisp(screenRect, facing, pose, phase, art, alpha, main, accent);
        break;
    case ArtStyle::Humanoid:
    case ArtStyle::Knight:
    case ArtStyle::Imp:
    default:
        DrawHumanoid(screenRect, facing, pose, phase, art, alpha, main, accent, motionVariant);
        break;
    }
}

} // namespace ecl
