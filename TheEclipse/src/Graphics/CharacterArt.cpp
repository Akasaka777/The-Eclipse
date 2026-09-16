#include "Graphics/CharacterArt.h"

#include "Common/MathUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

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

// ポーズから腕（武器）の角度を求める : 0 = 正面水平
float WeaponAngle(PoseKind pose, float phase, int facing)
{
    float deg = 0.0f;
    switch (pose) {
    case PoseKind::Idle:   deg = 70.0f + std::sin(phase * kTwoPi) * 4.0f; break;
    case PoseKind::Walk:   deg = 65.0f + std::sin(phase * kTwoPi) * 12.0f; break;
    case PoseKind::Run:    deg = 55.0f + std::sin(phase * kTwoPi) * 18.0f; break;
    case PoseKind::Jump:   deg = 35.0f; break;
    case PoseKind::Fall:   deg = 50.0f; break;
    case PoseKind::Dash:   deg = 95.0f; break;
    case PoseKind::Guard:  deg = 80.0f; break;
    case PoseKind::Hurt:   deg = 110.0f; break;
    case PoseKind::Dead:   deg = 95.0f; break;
    case PoseKind::Attack: {
        // 振りかぶり → 斬り下ろし
        const float t = math::Clamp(phase, 0.0f, 1.0f);
        if (t < 0.3f) {
            deg = math::Lerp(70.0f, -120.0f, t / 0.3f);
        } else if (t < 0.55f) {
            deg = math::Lerp(-120.0f, 25.0f, (t - 0.3f) / 0.25f);
        } else {
            deg = math::Lerp(25.0f, 70.0f, (t - 0.55f) / 0.45f);
        }
        break;
    }
    case PoseKind::Skill: {
        const float t = math::Clamp(phase, 0.0f, 1.0f);
        if (t < 0.25f) {
            deg = math::Lerp(70.0f, -140.0f, t / 0.25f);
        } else if (t < 0.45f) {
            deg = math::Lerp(-140.0f, 10.0f, (t - 0.25f) / 0.2f);
        } else if (t < 0.7f) {
            deg = math::Lerp(10.0f, -60.0f, (t - 0.45f) / 0.25f);
        } else {
            deg = math::Lerp(-60.0f, 70.0f, (t - 0.7f) / 0.3f);
        }
        break;
    }
    default: deg = 70.0f; break;
    }

    // facing = -1 のときは左右反転
    const float rad = math::DegToRad(deg);
    return (facing >= 0) ? rad : (math::DegToRad(180.0f) - rad);
}

//------------------------------------------------------------------------------
// 人型（プレイヤー / 騎士 / 小型）の代替描画
//------------------------------------------------------------------------------
void DrawHumanoid(const Rect& rect, int facing, PoseKind pose, float phase,
                  const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent)
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
    case PoseKind::Attack:
        lean = dir * h * (0.04f + math::Clamp(phase - 0.3f, 0.0f, 0.3f) * 0.4f);
        crouch = h * 0.03f;
        break;
    case PoseKind::Skill:
        lean = dir * h * (0.06f + math::Clamp(phase - 0.25f, 0.0f, 0.35f) * 0.5f);
        crouch = h * 0.05f;
        break;
    case PoseKind::Dead:
        break;
    default:
        break;
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
    const float angle = WeaponAngle(pose, phase, facing);
    // 腕は武器角度に追従（下向きを 90 度とする座標系に合わせる）
    const float armAngle = angle - math::DegToRad(dir > 0.0f ? 20.0f : -20.0f);
    const Vec2 hand = Rotate(shoulder, armLen, armAngle);
    Limb(shoulder, hand, h * 0.045f, main.Scaled(1.1f), alpha);

    if (art.hasWeapon) {
        const float weaponLen = h * 0.46f * WeaponReachScale(art.weapon);
        DrawWeapon(hand, angle, facing, weaponLen, art.weapon, accent, art.trim, alpha);

        // スキル発動中は軌跡を光らせる
        if (pose == PoseKind::Skill) {
            const Vec2 tip = Rotate(hand, weaponLen, angle);
            draw::Glow(tip.x, tip.y, h * 0.16f, art.trim, 150, 4);
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
void DrawWeapon(const Vec2& handPos, float angleRad, int facing, float length,
                WeaponType type, const ColorRGB& metal, const ColorRGB& accent, int alpha)
{
    // 向きは angleRad に含まれているため facing は形状調整用にのみ使う
    (void)facing;
    const Vec2 dirVec(std::cos(angleRad), std::sin(angleRad));
    const Vec2 perp(-dirVec.y, dirVec.x);

    auto point = [&](float along, float side) {
        return Vec2(handPos.x + dirVec.x * along + perp.x * side,
                    handPos.y + dirVec.y * along + perp.y * side);
    };

    switch (type) {
    case WeaponType::OneHandSword: {
        const float bladeW = length * 0.085f;
        // 柄
        draw::Line(point(-length * 0.14f, 0.0f).x, point(-length * 0.14f, 0.0f).y,
                   point(0.0f, 0.0f).x, point(0.0f, 0.0f).y, metal.Scaled(0.5f), length * 0.07f, alpha);
        // 鍔
        draw::Line(point(0.02f * length, -bladeW * 1.8f).x, point(0.02f * length, -bladeW * 1.8f).y,
                   point(0.02f * length, bladeW * 1.8f).x, point(0.02f * length, bladeW * 1.8f).y,
                   accent.Scaled(0.85f), length * 0.05f, alpha);
        // 刀身
        const Vec2 tip = point(length, 0.0f);
        draw::Triangle(point(length * 0.08f, -bladeW), point(length * 0.08f, bladeW), tip, metal, true, alpha);
        draw::Line(point(length * 0.1f, 0.0f).x, point(length * 0.1f, 0.0f).y, tip.x, tip.y,
                   palette::kWhite, length * 0.02f, math::ClampInt(alpha - 60, 0, 255));
        break;
    }
    case WeaponType::OneHandMace: {
        // 柄
        draw::Line(point(-length * 0.12f, 0.0f).x, point(-length * 0.12f, 0.0f).y,
                   point(length * 0.72f, 0.0f).x, point(length * 0.72f, 0.0f).y,
                   metal.Scaled(0.55f), length * 0.07f, alpha);
        // 頭部
        const Vec2 head = point(length * 0.82f, 0.0f);
        draw::Circle(head.x, head.y, length * 0.17f, metal, true, 1.0f, alpha);
        draw::Circle(head.x, head.y, length * 0.17f, accent.Scaled(0.7f), false, 2.0f, alpha);
        // 突起
        for (int i = 0; i < 4; ++i) {
            const float a = angleRad + static_cast<float>(i) * (kTwoPi / 4.0f) + 0.4f;
            draw::Line(head.x, head.y, head.x + std::cos(a) * length * 0.25f,
                       head.y + std::sin(a) * length * 0.25f, metal.Scaled(1.1f), length * 0.05f, alpha);
        }
        break;
    }
    case WeaponType::Dagger: {
        const float len = length * 0.55f;
        const float bladeW = len * 0.12f;
        draw::Line(point(-len * 0.16f, 0.0f).x, point(-len * 0.16f, 0.0f).y,
                   point(0.0f, 0.0f).x, point(0.0f, 0.0f).y, metal.Scaled(0.5f), len * 0.10f, alpha);
        draw::Triangle(point(0.0f, -bladeW), point(0.0f, bladeW), point(len, 0.0f), metal, true, alpha);
        draw::Circle(point(0.0f, 0.0f).x, point(0.0f, 0.0f).y, len * 0.07f, accent, true, 1.0f, alpha);
        break;
    }
    case WeaponType::Rapier: {
        const float len = length * 1.12f;
        // 護拳
        const Vec2 guard = point(0.04f * len, 0.0f);
        draw::Circle(guard.x, guard.y, len * 0.07f, accent.Scaled(0.9f), false, 3.0f, alpha);
        // 細身の刀身
        draw::Line(guard.x, guard.y, point(len, 0.0f).x, point(len, 0.0f).y, metal, len * 0.035f, alpha);
        draw::Line(point(-len * 0.12f, 0.0f).x, point(-len * 0.12f, 0.0f).y, guard.x, guard.y,
                   metal.Scaled(0.5f), len * 0.055f, alpha);
        break;
    }
    case WeaponType::Spear: {
        const float len = length * 1.30f;
        // 石突〜柄
        draw::Line(point(-len * 0.30f, 0.0f).x, point(-len * 0.30f, 0.0f).y,
                   point(len * 0.80f, 0.0f).x, point(len * 0.80f, 0.0f).y,
                   ColorRGB(120, 88, 58), len * 0.035f, alpha);
        // 穂先
        draw::Triangle(point(len * 0.78f, -len * 0.06f), point(len * 0.78f, len * 0.06f),
                       point(len, 0.0f), metal, true, alpha);
        draw::Line(point(len * 0.74f, -len * 0.05f).x, point(len * 0.74f, -len * 0.05f).y,
                   point(len * 0.74f, len * 0.05f).x, point(len * 0.74f, len * 0.05f).y,
                   accent.Scaled(0.8f), len * 0.03f, alpha);
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
               const ActorArt& art, const Animator* animator, int alpha, float flash)
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
        DrawHumanoid(screenRect, facing, pose, phase, art, alpha, main, accent);
        break;
    }
}

} // namespace ecl
