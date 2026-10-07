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
// 描画の共通部品
//   どのモデルも「暗い縁取り → 地の色 → 影 → 明るい面」の順に重ねて立体感を出す。
//   光は画面の左上から当たる想定。
//------------------------------------------------------------------------------
const ColorRGB kInk(14, 16, 24);   // 縁取りの色

// 凸に近い多角形（中心から扇状に塗る）
//   アンチエイリアスのかかった三角形を並べると、継ぎ目に下の色（縁取り）が細い線で
//   透けるので、継ぎ目のある図形は 2 回重ねて塗って透けを消す。
void Poly(const std::vector<Vec2>& pts, const ColorRGB& color, int alpha)
{
    if (pts.size() < 3) return;
    Vec2 c(0.0f, 0.0f);
    for (const Vec2& p : pts) c += p;
    c *= 1.0f / static_cast<float>(pts.size());
    const int passes = (pts.size() > 3 && alpha >= 255) ? 2 : 1;
    for (int pass = 0; pass < passes; ++pass) {
        for (size_t i = 0; i < pts.size(); ++i) {
            draw::Triangle(c, pts[i], pts[(i + 1) % pts.size()], color, true, alpha);
        }
    }
}

// 多角形を中心から外へ amount だけ太らせる（縁取り用）
std::vector<Vec2> Grow(const std::vector<Vec2>& pts, float amount)
{
    Vec2 c(0.0f, 0.0f);
    for (const Vec2& p : pts) c += p;
    c *= 1.0f / static_cast<float>(pts.size());
    std::vector<Vec2> out;
    out.reserve(pts.size());
    for (const Vec2& p : pts) {
        Vec2 d = p - c;
        const float len = d.Length();
        if (len > 0.001f) d *= (len + amount) / len;
        out.push_back(c + d);
    }
    return out;
}

// 縁取り付きの多角形
void PolyInk(const std::vector<Vec2>& pts, const ColorRGB& fill, float ink, int alpha)
{
    Poly(Grow(pts, ink), kInk, alpha);
    Poly(pts, fill, alpha);
}

// 縁取り付きの円
void Ball(const Vec2& c, float r, const ColorRGB& fill, float ink, int alpha)
{
    draw::Circle(c.x, c.y, r + ink, kInk, true, 1.0f, alpha);
    draw::Circle(c.x, c.y, r, fill, true, 1.0f, alpha);
}

// 縁取り付きの太い線（両端は丸い）
void Capsule(const Vec2& a, const Vec2& b, float r, const ColorRGB& fill, float ink, int alpha)
{
    draw::Line(a.x, a.y, b.x, b.y, kInk, (r + ink) * 2.0f, alpha);
    draw::Circle(a.x, a.y, r + ink, kInk, true, 1.0f, alpha);
    draw::Circle(b.x, b.y, r + ink, kInk, true, 1.0f, alpha);
    draw::Line(a.x, a.y, b.x, b.y, fill, r * 2.0f, alpha);
    draw::Circle(a.x, a.y, r, fill, true, 1.0f, alpha);
    draw::Circle(b.x, b.y, r, fill, true, 1.0f, alpha);
    // 光の当たる側に細いハイライト
    const Vec2 d = b - a;
    const float len = d.Length();
    if (len > 1.0f && r > 2.0f) {
        Vec2 n(-d.y / len, d.x / len);
        if (n.y > 0.0f) n = -n;   // 上を向いた側
        const Vec2 off = n * (r * 0.45f);
        draw::Line(a.x + off.x, a.y + off.y, b.x + off.x, b.y + off.y, fill.Scaled(1.3f),
                   math::MaxF(1.0f, r * 0.45f), alpha);
    }
}

// 2 本の骨でつながる関節の位置（膝・肘）
//   bendX : 関節を曲げる向き（+1 で右、-1 で左）。届かないときは伸ばし切る
Vec2 Joint(const Vec2& root, const Vec2& end, float len1, float len2, float bendX)
{
    const Vec2 d = end - root;
    float dist = d.Length();
    if (dist < 0.001f) return root + Vec2(bendX * len1, 0.0f);
    dist = math::MinF(dist, len1 + len2 - 0.01f);
    const float a = (len1 * len1 - len2 * len2 + dist * dist) / (2.0f * dist);
    const float hgt = std::sqrt(math::MaxF(0.0f, len1 * len1 - a * a));
    const Vec2 u = d * (1.0f / d.Length());
    Vec2 n(-u.y, u.x);
    if (n.x * bendX < 0.0f) n = -n;
    return root + u * a + n * hgt;
}

// 岩の塊（面取りした箱：上と左を明るく、右と下を暗く）
void RockBlock(const Rect& r, const ColorRGB& base, float ink, int alpha)
{
    const float bevel = math::MinF(r.Width(), r.Height()) * 0.16f;
    draw::FillRect(r.Expanded(ink), kInk, alpha);
    draw::FillRect(r, base, alpha);
    draw::FillRect(Rect(r.left, r.top, r.right, r.top + bevel), base.Scaled(1.3f), alpha);
    draw::FillRect(Rect(r.left, r.top, r.left + bevel, r.bottom), base.Scaled(1.15f), alpha);
    draw::FillRect(Rect(r.right - bevel, r.top + bevel, r.right, r.bottom), base.Scaled(0.72f), alpha);
    draw::FillRect(Rect(r.left + bevel, r.bottom - bevel, r.right, r.bottom), base.Scaled(0.62f), alpha);
}

//------------------------------------------------------------------------------
// 人型（プレイヤー / 騎士 / 小型）の代替描画
//   膝と肘で曲がる手足、肩幅のある胴、ベルトと腰当て、肩当て、
//   髪（騎士は兜と羽根飾り、小型は角）を描き分ける。
//   武器を持つ手の位置と角度の計算は武器モーションと共通なので変えない。
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
    const float ink = math::MaxF(1.0f, h * 0.010f);
    const bool knight = (art.style == ArtStyle::Knight);
    const bool imp = (art.style == ArtStyle::Imp);

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

    // 色
    const ColorRGB skin = imp ? ColorRGB::Lerp(main, accent, 0.35f)
                              : ColorRGB::Lerp(accent, ColorRGB(236, 196, 168), 0.55f);
    const ColorRGB cloth = main;
    const ColorRGB clothDark = main.Scaled(0.70f);
    const ColorRGB armor = ColorRGB::Lerp(accent.Scaled(0.80f), main, 0.25f);
    const ColorRGB leather(84, 58, 42);
    const ColorRGB boots = ColorRGB::Lerp(main.Scaled(0.55f), leather, 0.5f);

    // --- 死亡時は横たわった表現 ---------------------------------------------
    if (pose == PoseKind::Dead) {
        const float fall = math::Clamp(phase, 0.0f, 1.0f);
        const float lieY = bottom - math::Lerp(h * 0.45f, h * 0.09f, fall);
        const float span = math::Lerp(h * 0.25f, h * 0.55f, fall);
        const Vec2 headC(cx - dir * span, lieY - h * 0.02f);
        const Vec2 hipC(cx + dir * span * 0.25f, lieY + h * 0.02f);
        Capsule(hipC, Vec2(cx + dir * span * 1.05f, bottom - h * 0.04f), h * 0.030f, clothDark, ink, alpha);
        Capsule(hipC, Vec2(cx + dir * span * 0.95f, bottom - h * 0.02f), h * 0.030f, boots, ink, alpha);
        PolyInk({ Vec2(headC.x + dir * h * 0.08f, lieY - h * 0.07f), Vec2(hipC.x, lieY - h * 0.05f),
                  Vec2(hipC.x, lieY + h * 0.06f), Vec2(headC.x + dir * h * 0.08f, lieY + h * 0.06f) },
                cloth, ink, alpha);
        Ball(headC, h * 0.09f, skin, ink, alpha);
        return;
    }

    const float hipY = bottom - h * 0.46f + crouch;
    const float shoulderY = bottom - h * 0.74f + crouch;
    const float headR = h * 0.105f;
    const float headY = bottom - h * 0.84f + crouch - headR * 0.4f;
    const float torsoW = w * 0.52f;

    // --- 腕の位置（武器モーションと共通の計算）---------------------------------
    const Vec2 shoulder(cx + dir * torsoW * 0.35f + lean * 0.7f, shoulderY + h * 0.05f);
    const float armLen = h * 0.20f;
    const float angle = ToWeaponAngle(swing.angleDeg, facing);
    const float armTwist = math::DegToRad(dir > 0.0f ? 20.0f : -20.0f);
    // 突き技は肩から手までまとめて前へ送り出す
    const Vec2 handOffset(dir * h * swing.reachOut, -h * swing.liftUp);
    Vec2 hand = Rotate(shoulder, armLen, angle - armTwist);
    hand.x += handOffset.x;
    hand.y += handOffset.y;

    // --- 奥の腕（盾を持つ / 歩きで振る）---------------------------------------
    const Vec2 backShoulder(cx - dir * torsoW * 0.30f + lean * 0.55f, shoulderY + h * 0.06f);
    if (!art.hasOffHandWeapon) {
        Vec2 backHand(backShoulder.x - dir * h * 0.02f - legSwing * 0.35f, backShoulder.y + h * 0.21f);
        if (art.hasShield) {
            backHand = Vec2(cx - dir * torsoW * 0.45f + lean * 0.5f
                                + ((pose == PoseKind::Guard) ? dir * torsoW * 1.5f : 0.0f),
                            shoulderY + h * 0.12f);
        }
        const Vec2 elbow = Joint(backShoulder, backHand, h * 0.11f, h * 0.11f, -dir);
        Capsule(backShoulder, elbow, h * 0.024f, clothDark.Scaled(0.85f), ink, alpha);
        Capsule(elbow, backHand, h * 0.021f, clothDark.Scaled(0.85f), ink, alpha);
        Ball(backHand, h * 0.024f, armor.Scaled(0.7f), ink, alpha);
    }

    // --- マント（騎士のみ・体の後ろ）-----------------------------------------
    if (knight) {
        const float sway = std::sin(phase * kTwoPi) * h * 0.02f - lean * 0.6f;
        const float capeTop = shoulderY - h * 0.01f;
        const float capeX = cx - dir * torsoW * 0.15f + lean * 0.5f;
        const float hemY = bottom - h * 0.10f;
        const ColorRGB cape = art.trim.Scaled(0.45f);
        std::vector<Vec2> pts = {
            Vec2(capeX + dir * torsoW * 0.30f, capeTop),
            Vec2(capeX - dir * torsoW * 0.45f, capeTop + h * 0.02f),
            Vec2(capeX - dir * (torsoW * 0.75f + w * 0.10f) + sway, hemY),
            Vec2(capeX - dir * torsoW * 0.30f + sway * 0.6f, hemY + h * 0.03f),
            Vec2(capeX + dir * torsoW * 0.10f + sway * 0.3f, hemY - h * 0.02f),
        };
        PolyInk(pts, cape, ink, alpha);
        // 折り目
        for (int i = 0; i < 3; ++i) {
            const float t = 0.25f + 0.22f * static_cast<float>(i);
            const Vec2 top(math::Lerp(pts[0].x, pts[1].x, t), capeTop + h * 0.04f);
            const Vec2 low(math::Lerp(pts[2].x, pts[4].x, t), hemY - h * 0.01f);
            draw::Line(top.x, top.y, low.x, low.y, cape.Scaled(0.65f), math::MaxF(1.0f, h * 0.006f), alpha);
        }
        // 裏地
        Poly({ pts[1], pts[2], pts[3] }, art.trim.Scaled(0.30f), alpha);
    }

    // --- 脚（膝で曲がる。奥の脚は暗く）-----------------------------------------
    const Vec2 hip(cx + lean * 0.4f, hipY);
    // 太もも＋すねの長さは、立ったときに膝がわずかに曲がる程度（腰〜足首の 1.06 倍）
    const float thigh = h * (0.46f - 0.035f) * 0.53f;
    auto drawLeg = [&](float footX, bool back) {
        const Vec2 hipJoint(hip.x + (back ? -dir : dir) * w * 0.06f, hip.y);
        const Vec2 ankle(footX, bottom - h * 0.035f);
        const Vec2 knee = Joint(hipJoint, ankle, thigh, thigh, dir);
        const ColorRGB pants = back ? clothDark.Scaled(0.75f) : clothDark;
        const ColorRGB boot = back ? boots.Scaled(0.75f) : boots;
        Capsule(hipJoint, knee, h * 0.033f, pants, ink, alpha);
        Capsule(knee, ankle, h * 0.028f, boot, ink, alpha);
        // 膝当て
        if (!imp) Ball(knee, h * 0.022f, back ? armor.Scaled(0.7f) : armor, ink, alpha);
        // ブーツ（つま先を向いている方へ）
        PolyInk({ Vec2(ankle.x - dir * h * 0.030f, ankle.y - h * 0.020f),
                  Vec2(ankle.x + dir * h * 0.025f, ankle.y - h * 0.018f),
                  Vec2(ankle.x + dir * h * 0.072f, bottom - h * 0.006f),
                  Vec2(ankle.x - dir * h * 0.034f, bottom) },
                boot, ink, alpha);
    };
    drawLeg(cx - legSwing * 0.8f + lean * 0.2f, true);
    drawLeg(cx + legSwing * 0.8f + lean * 0.2f, false);

    // --- 胴 -----------------------------------------------------------------
    const float tx = cx + lean * 0.5f;
    const float waistY = hipY + h * 0.03f;
    // 腰当て（腰から下に広がる）
    PolyInk({ Vec2(tx - torsoW * 0.42f, waistY - h * 0.03f), Vec2(tx + torsoW * 0.42f, waistY - h * 0.03f),
              Vec2(tx + torsoW * 0.55f, waistY + h * 0.11f), Vec2(tx - torsoW * 0.55f, waistY + h * 0.11f) },
            knight ? armor.Scaled(0.85f) : cloth.Scaled(0.85f), ink, alpha);
    draw::Line(tx, waistY, tx + dir * torsoW * 0.04f, waistY + h * 0.10f, kInk, math::MaxF(1.0f, ink), alpha);

    const std::vector<Vec2> torso = {
        Vec2(tx - torsoW * 0.56f, shoulderY), Vec2(tx + torsoW * 0.56f, shoulderY),
        Vec2(tx + torsoW * 0.42f, waistY), Vec2(tx - torsoW * 0.42f, waistY),
    };
    PolyInk(torso, cloth, ink, alpha);
    // 影の面（奥側）と明るい面（手前側）
    Poly({ Vec2(tx - dir * torsoW * 0.56f, shoulderY), Vec2(tx - dir * torsoW * 0.18f, shoulderY),
           Vec2(tx - dir * torsoW * 0.12f, waistY), Vec2(tx - dir * torsoW * 0.42f, waistY) },
         cloth.Scaled(0.72f), alpha);
    // 胸当て
    const ColorRGB plate = knight ? armor : ColorRGB::Lerp(cloth, accent, 0.30f);
    PolyInk({ Vec2(tx - dir * torsoW * 0.10f, shoulderY + h * 0.02f),
              Vec2(tx + dir * torsoW * 0.48f, shoulderY + h * 0.02f),
              Vec2(tx + dir * torsoW * 0.36f, shoulderY + h * 0.15f),
              Vec2(tx - dir * torsoW * 0.04f, shoulderY + h * 0.17f) },
            plate, ink * 0.8f, alpha);
    draw::Line(tx + dir * torsoW * 0.02f, shoulderY + h * 0.05f, tx + dir * torsoW * 0.38f,
               shoulderY + h * 0.04f, plate.Scaled(1.35f), math::MaxF(1.0f, h * 0.008f), alpha);
    // 飾り線（アクセント色）
    draw::Line(tx - torsoW * 0.44f, shoulderY + h * 0.20f, tx + torsoW * 0.44f, shoulderY + h * 0.22f,
               art.trim, math::MaxF(1.0f, h * 0.010f), alpha);
    // ベルトとバックル
    const float beltY = waistY - h * 0.035f;
    draw::Line(tx - torsoW * 0.44f, beltY, tx + torsoW * 0.44f, beltY, kInk, h * 0.034f, alpha);
    draw::Line(tx - torsoW * 0.42f, beltY, tx + torsoW * 0.42f, beltY, leather, h * 0.022f, alpha);
    const Rect buckle = Rect::FromCenter(tx + dir * torsoW * 0.10f, beltY, h * 0.036f, h * 0.030f);
    draw::FillRect(buckle.Expanded(ink * 0.6f), kInk, alpha);
    draw::FillRect(buckle, art.trim.Scaled(0.9f), alpha);

    // --- 首と頭 -----------------------------------------------------------------
    const float headX = cx + lean * 0.9f + dir * w * 0.04f;
    Capsule(Vec2(headX - dir * headR * 0.15f, shoulderY - h * 0.005f),
            Vec2(headX - dir * headR * 0.10f, headY + headR * 0.6f), h * 0.026f, skin.Scaled(0.85f), ink, alpha);

    if (knight) {
        // 兜：丸い鉢・顔を覆う面頬・光る覗き穴・羽根飾り
        const ColorRGB helm = armor;
        const float plume = std::sin(phase * kTwoPi) * h * 0.01f;
        PolyInk({ Vec2(headX - dir * headR * 0.2f, headY - headR * 0.9f),
                  Vec2(headX - dir * headR * 1.9f, headY - headR * 1.2f + plume),
                  Vec2(headX - dir * headR * 1.4f, headY - headR * 0.4f + plume) },
                art.trim.Scaled(0.75f), ink, alpha);
        Ball(Vec2(headX, headY), headR * 1.05f, helm, ink, alpha);
        Poly({ Vec2(headX - dir * headR * 0.9f, headY - headR * 0.2f), Vec2(headX - dir * headR * 0.2f, headY - headR * 1.0f),
               Vec2(headX - dir * headR * 0.95f, headY + headR * 0.5f) }, helm.Scaled(0.7f), alpha);
        PolyInk({ Vec2(headX - dir * headR * 0.05f, headY - headR * 0.05f),
                  Vec2(headX + dir * headR * 1.12f, headY - headR * 0.05f),
                  Vec2(headX + dir * headR * 1.00f, headY + headR * 0.85f),
                  Vec2(headX + dir * headR * 0.05f, headY + headR * 0.95f) },
                helm.Scaled(0.85f), ink * 0.7f, alpha);
        draw::Line(headX + dir * headR * 0.15f, headY + headR * 0.20f, headX + dir * headR * 1.02f,
                   headY + headR * 0.20f, kInk, headR * 0.26f, alpha);
        draw::Line(headX + dir * headR * 0.30f, headY + headR * 0.20f, headX + dir * headR * 0.95f,
                   headY + headR * 0.20f, art.trim, math::MaxF(1.0f, headR * 0.10f), alpha);
        draw::Circle(headX - dir * headR * 0.35f, headY - headR * 0.45f, headR * 0.22f, helm.Scaled(1.35f),
                     true, 1.0f, alpha);
    } else {
        // 顔
        Ball(Vec2(headX, headY), headR, skin, ink, alpha);
        // 顎から首の影
        draw::Circle(headX - dir * headR * 0.35f, headY + headR * 0.35f, headR * 0.45f, skin.Scaled(0.85f),
                     true, 1.0f, alpha);
        draw::Circle(headX + dir * headR * 0.05f, headY - headR * 0.05f, headR * 0.72f, skin, true, 1.0f, alpha);
        // 目（白目＋瞳）
        const Vec2 eye(headX + dir * headR * 0.48f, headY - headR * 0.05f);
        if (imp) {
            draw::Glow(eye.x, eye.y, headR * 0.45f, art.trim, 140, 3);
            draw::Circle(eye.x, eye.y, headR * 0.20f, art.trim, true, 1.0f, alpha);
            draw::Circle(eye.x, eye.y, headR * 0.09f, palette::kWhite, true, 1.0f, alpha);
        } else {
            draw::Circle(eye.x, eye.y, headR * 0.17f, palette::kWhite, true, 1.0f, alpha);
            draw::Circle(eye.x + dir * headR * 0.04f, eye.y, headR * 0.11f, art.trim, true, 1.0f, alpha);
            draw::Line(eye.x - dir * headR * 0.18f, eye.y - headR * 0.25f, eye.x + dir * headR * 0.20f,
                       eye.y - headR * 0.28f, kInk, math::MaxF(1.0f, headR * 0.08f), alpha);
        }
        if (imp) {
            // 角と尖った耳
            for (float k : { -0.55f, 0.15f }) {
                PolyInk({ Vec2(headX + dir * headR * (k - 0.18f), headY - headR * 0.75f),
                          Vec2(headX + dir * headR * (k + 0.18f), headY - headR * 0.80f),
                          Vec2(headX + dir * headR * (k - 0.35f), headY - headR * 1.75f) },
                        accent.Scaled(0.85f), ink * 0.7f, alpha);
            }
            PolyInk({ Vec2(headX - dir * headR * 0.70f, headY - headR * 0.20f),
                      Vec2(headX - dir * headR * 0.80f, headY + headR * 0.25f),
                      Vec2(headX - dir * headR * 1.55f, headY - headR * 0.40f) },
                    skin.Scaled(0.9f), ink * 0.7f, alpha);
            draw::Line(headX + dir * headR * 0.25f, headY + headR * 0.45f, headX + dir * headR * 0.80f,
                       headY + headR * 0.38f, kInk, math::MaxF(1.0f, headR * 0.10f), alpha);
        } else {
            // 髪：後頭部を覆い、前髪を数房たらす
            const ColorRGB hair = ColorRGB::Lerp(main.Scaled(0.45f), kInk, 0.3f);
            PolyInk({ Vec2(headX - dir * headR * 1.08f, headY + headR * 0.35f),
                      Vec2(headX - dir * headR * 1.00f, headY - headR * 0.55f),
                      Vec2(headX - dir * headR * 0.35f, headY - headR * 1.12f),
                      Vec2(headX + dir * headR * 0.55f, headY - headR * 1.02f),
                      Vec2(headX + dir * headR * 1.05f, headY - headR * 0.40f),
                      Vec2(headX + dir * headR * 0.20f, headY - headR * 0.35f),
                      Vec2(headX - dir * headR * 0.30f, headY + headR * 0.10f) },
                    hair, ink, alpha);
            for (int i = 0; i < 3; ++i) {
                const float bx = headX + dir * headR * (0.15f + 0.30f * static_cast<float>(i));
                Poly({ Vec2(bx - dir * headR * 0.20f, headY - headR * 0.70f),
                       Vec2(bx + dir * headR * 0.20f, headY - headR * 0.75f),
                       Vec2(bx + dir * headR * 0.05f, headY - headR * 0.15f) }, hair, alpha);
            }
            draw::Line(headX - dir * headR * 0.55f, headY - headR * 0.85f, headX + dir * headR * 0.25f,
                       headY - headR * 0.95f, hair.Scaled(1.8f), math::MaxF(1.0f, headR * 0.10f), alpha);
        }
    }

    // --- 盾 -----------------------------------------------------------------
    if (art.hasShield) {
        const float shieldX = cx - dir * (torsoW * 0.55f) + lean * 0.5f
                            + ((pose == PoseKind::Guard) ? dir * torsoW * 1.5f : 0.0f);
        const float shieldY = shoulderY + h * 0.10f;
        const float sw = w * 0.17f;
        const float sh = h * 0.14f;
        const std::vector<Vec2> kite = {
            Vec2(shieldX - sw, shieldY - sh), Vec2(shieldX + sw, shieldY - sh),
            Vec2(shieldX + sw * 0.95f, shieldY + sh * 0.25f), Vec2(shieldX, shieldY + sh * 1.15f),
            Vec2(shieldX - sw * 0.95f, shieldY + sh * 0.25f),
        };
        PolyInk(kite, ColorRGB::Lerp(accent, main, 0.4f), ink, alpha);
        PolyInk(Grow(kite, -math::MaxF(2.0f, sw * 0.18f)), main.Scaled(1.25f), ink * 0.5f, alpha);
        // 紋章（ひし形）
        PolyInk({ Vec2(shieldX, shieldY - sh * 0.55f), Vec2(shieldX + sw * 0.35f, shieldY),
                  Vec2(shieldX, shieldY + sh * 0.55f), Vec2(shieldX - sw * 0.35f, shieldY) },
                art.trim, ink * 0.5f, alpha);
        draw::Line(shieldX - sw * 0.75f, shieldY - sh * 0.80f, shieldX - sw * 0.15f, shieldY - sh * 0.80f,
                   palette::kWhite, math::MaxF(1.0f, h * 0.006f), alpha / 2);
    }

    // --- 肩当て（手前） ---------------------------------------------------------
    {
        const Vec2 pad(shoulder.x - dir * torsoW * 0.04f, shoulder.y - h * 0.03f);
        const float pr = h * (knight ? 0.060f : 0.048f);
        PolyInk({ Vec2(pad.x - pr * 1.1f, pad.y + pr * 0.5f), Vec2(pad.x - pr * 0.8f, pad.y - pr * 0.7f),
                  Vec2(pad.x + pr * 0.3f, pad.y - pr * 1.0f), Vec2(pad.x + pr * 1.15f, pad.y - pr * 0.2f),
                  Vec2(pad.x + pr * 1.1f, pad.y + pr * 0.6f) },
                knight ? armor : ColorRGB::Lerp(armor, cloth, 0.35f), ink, alpha);
        draw::Line(pad.x - pr * 0.9f, pad.y + pr * 0.45f, pad.x + pr * 1.0f, pad.y + pr * 0.5f, art.trim,
                   math::MaxF(1.0f, h * 0.007f), alpha);
    }

    // --- 腕（肘で曲がる）と武器 ---------------------------------------------------
    {
        const Vec2 elbow = Joint(shoulder, hand, armLen * 0.55f, armLen * 0.55f + h * swing.reachOut * 0.5f,
                                 -dir);
        Capsule(shoulder, elbow, h * 0.026f, cloth.Scaled(1.05f), ink, alpha);
        Capsule(elbow, hand, h * 0.023f, ColorRGB::Lerp(armor, cloth, 0.3f), ink, alpha);
        Ball(hand, h * 0.025f, armor, ink, alpha);
    }

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
        // 握った手を武器の柄の上に重ねる
        Ball(hand, h * 0.022f, armor, ink * 0.8f, alpha);

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
            const Vec2 offElbow = Joint(offShoulder, offHand, armLen * 0.55f,
                                        armLen * 0.55f + h * swing.reachOut * 0.4f, -dir);
            Capsule(offShoulder, offElbow, h * 0.024f, cloth, ink, alpha);
            Capsule(offElbow, offHand, h * 0.021f, ColorRGB::Lerp(armor, cloth, 0.4f), ink, alpha);
            DrawWeapon(offHand, offAngle, facing, weaponLen * 0.95f, art.weapon,
                       accent.Scaled(0.9f), art.trim, alpha);
            Ball(offHand, h * 0.021f, armor.Scaled(0.9f), ink * 0.8f, alpha);

            if (pose == PoseKind::Skill) {
                const Vec2 tip = Rotate(offHand, weaponLen * 0.95f, offAngle);
                draw::Glow(tip.x, tip.y, h * 0.14f, art.trim, 130, 4);
            }
        }
    }
}

//------------------------------------------------------------------------------
// 四足獣（狼 / トカゲ / 狼王）
//   胸の厚い胴、関節で曲がる 4 本の脚と爪、開く顎と牙、毛並みのたてがみ、
//   ふさふさの尻尾。背中のトゲはアクセント色。
//------------------------------------------------------------------------------
void DrawBeast(const Rect& rect, int facing, PoseKind pose, float phase,
               const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent)
{
    const float w = rect.Width();
    const float h = rect.Height();
    const float cx = rect.CenterX();
    const float bottom = rect.bottom;
    const float dir = (facing >= 0) ? 1.0f : -1.0f;
    const float ink = math::MaxF(1.0f, h * 0.012f);

    const ColorRGB belly = ColorRGB::Lerp(main, accent, 0.35f);
    const ColorRGB dark = main.Scaled(0.62f);

    if (pose == PoseKind::Dead) {
        const float fall = math::Clamp(phase, 0.0f, 1.0f);
        const float lieY = bottom - h * math::Lerp(0.40f, 0.16f, fall);
        PolyInk({ Vec2(cx - w * 0.45f, lieY), Vec2(cx - w * 0.10f, lieY - h * 0.16f),
                  Vec2(cx + w * 0.40f, lieY - h * 0.10f), Vec2(cx + w * 0.50f, bottom - h * 0.02f),
                  Vec2(cx - w * 0.48f, bottom - h * 0.02f) },
                main.Scaled(0.75f), ink, alpha);
        Ball(Vec2(cx + dir * w * 0.55f, bottom - h * 0.10f), h * 0.12f, main.Scaled(0.8f), ink, alpha);
        return;
    }

    float legSwing = 0.0f;
    float lunge = 0.0f;
    float jaw = 0.0f;   // 顎の開き（0〜1）
    if (pose == PoseKind::Walk || pose == PoseKind::Run) {
        legSwing = std::sin(phase * kTwoPi) * h * 0.16f;
    } else if (pose == PoseKind::Attack || pose == PoseKind::Skill) {
        const float t = std::sin(math::Clamp(phase, 0.0f, 1.0f) * math::kPi);
        lunge = dir * w * 0.18f * t;
        jaw = t;
    } else if (pose == PoseKind::Idle) {
        lunge = std::sin(phase * kTwoPi) * h * 0.01f;
    }
    const float breathe = std::sin(phase * kTwoPi) * h * 0.008f;

    const float top = bottom - h * 0.78f + breathe;          // 背中の高さ
    const float under = bottom - h * 0.30f;                  // 腹の高さ
    auto X = [&](float k) { return cx + lunge + dir * w * k; };

    // --- 脚（奥の 2 本を先に暗く描く）--------------------------------------------
    auto drawLeg = [&](float baseK, float swingAmount, bool front, bool back) {
        const ColorRGB color = back ? dark.Scaled(0.75f) : main.Scaled(0.9f);
        const Vec2 rootP(X(baseK), under - h * 0.06f);
        const Vec2 paw(X(baseK) + swingAmount, bottom - h * 0.03f);
        // 前脚は膝が後ろ、後ろ脚は踵が後ろへ曲がる
        const float len = (paw.y - rootP.y) * 0.56f;
        const Vec2 knee = Joint(rootP, paw, len, len, front ? -dir : dir);
        Capsule(rootP, knee, h * 0.055f, color, ink, alpha);
        Capsule(knee, paw, h * 0.036f, color, ink, alpha);
        PolyInk({ Vec2(paw.x - dir * h * 0.04f, paw.y - h * 0.02f), Vec2(paw.x + dir * h * 0.07f, paw.y - h * 0.01f),
                  Vec2(paw.x + dir * h * 0.08f, bottom), Vec2(paw.x - dir * h * 0.05f, bottom) },
                color.Scaled(0.85f), ink, alpha);
        for (int i = 0; i < 2; ++i) {   // 爪
            const float tx = paw.x + dir * h * (0.05f + 0.035f * static_cast<float>(i));
            Poly({ Vec2(tx, bottom - h * 0.025f), Vec2(tx + dir * h * 0.03f, bottom),
                   Vec2(tx - dir * h * 0.005f, bottom) }, accent, alpha);
        }
    };
    drawLeg(0.22f, -legSwing, true, true);
    drawLeg(-0.30f, legSwing, false, true);

    // --- 尻尾（3 節でしなる）-----------------------------------------------------
    {
        const float wave = std::sin(phase * kTwoPi + 0.8f) * h * 0.08f;
        const Vec2 t0(X(-0.40f), top + h * 0.14f);
        const Vec2 t1(X(-0.56f), top + h * 0.02f + wave * 0.4f);
        const Vec2 t2(X(-0.70f), top - h * 0.10f + wave);
        const Vec2 t3(X(-0.80f), top - h * 0.14f + wave * 1.3f);
        PolyInk({ t0 + Vec2(0.0f, -h * 0.06f), t1 + Vec2(0.0f, -h * 0.07f), t2 + Vec2(0.0f, -h * 0.05f), t3,
                  t2 + Vec2(0.0f, h * 0.05f), t1 + Vec2(0.0f, h * 0.06f), t0 + Vec2(0.0f, h * 0.07f) },
                main.Scaled(0.85f), ink, alpha);
        Poly({ t2 + Vec2(0.0f, -h * 0.03f), t3, t2 + Vec2(0.0f, h * 0.03f) }, belly, alpha);
    }

    // --- 胴 -----------------------------------------------------------------
    const std::vector<Vec2> body = {
        Vec2(X(-0.42f), top + h * 0.20f), Vec2(X(-0.30f), top + h * 0.04f), Vec2(X(-0.02f), top + h * 0.03f),
        Vec2(X(0.20f), top - h * 0.04f), Vec2(X(0.36f), top + h * 0.06f), Vec2(X(0.40f), top + h * 0.26f),
        Vec2(X(0.28f), under + h * 0.02f), Vec2(X(0.02f), under - h * 0.02f), Vec2(X(-0.30f), under),
        Vec2(X(-0.44f), top + h * 0.36f),
    };
    PolyInk(body, main, ink, alpha);
    // 背中の影と腹の明るい毛
    Poly({ Vec2(X(-0.40f), top + h * 0.17f), Vec2(X(-0.30f), top + h * 0.06f), Vec2(X(0.20f), top - h * 0.02f),
           Vec2(X(0.18f), top + h * 0.09f), Vec2(X(-0.30f), top + h * 0.14f) },
         main.Scaled(0.78f), alpha);
    Poly({ Vec2(X(0.30f), top + h * 0.30f), Vec2(X(0.26f), under), Vec2(X(0.0f), under - h * 0.03f),
           Vec2(X(-0.22f), under - h * 0.02f), Vec2(X(0.0f), under - h * 0.12f) },
         belly, alpha);
    // 毛並み（短い筋）
    for (int i = 0; i < 5; ++i) {
        const float k = -0.28f + 0.11f * static_cast<float>(i);
        draw::Line(X(k), top + h * 0.12f, X(k - 0.05f), top + h * 0.20f, main.Scaled(0.6f),
                   math::MaxF(1.0f, h * 0.008f), alpha);
    }
    // 背中のトゲ
    for (int i = 0; i < 4; ++i) {
        const float k = -0.26f + 0.13f * static_cast<float>(i);
        const float y = top + h * (0.03f - 0.012f * static_cast<float>(i));
        PolyInk({ Vec2(X(k - 0.045f), y + h * 0.02f), Vec2(X(k + 0.045f), y + h * 0.01f),
                  Vec2(X(k - 0.02f), y - h * 0.12f) },
                art.trim, ink * 0.7f, alpha);
    }

    // --- 手前の脚 ---------------------------------------------------------------
    drawLeg(0.28f, legSwing, true, false);
    drawLeg(-0.24f, -legSwing, false, false);

    // --- 頭（たてがみ・耳・顎・牙・目）---------------------------------------------
    const float headTop = top - h * 0.10f;
    // たてがみ
    PolyInk({ Vec2(X(0.12f), top + h * 0.02f), Vec2(X(0.22f), headTop - h * 0.04f), Vec2(X(0.34f), headTop - h * 0.02f),
              Vec2(X(0.46f), top + h * 0.20f), Vec2(X(0.30f), top + h * 0.30f), Vec2(X(0.16f), top + h * 0.16f) },
            main.Scaled(0.72f), ink, alpha);
    // 奥の耳
    PolyInk({ Vec2(X(0.36f), headTop + h * 0.02f), Vec2(X(0.42f), headTop + h * 0.01f), Vec2(X(0.35f), headTop - h * 0.13f) },
            dark, ink * 0.7f, alpha);
    // 下顎（攻撃で開く）
    const float open = jaw * h * 0.10f;
    PolyInk({ Vec2(X(0.42f), headTop + h * 0.14f), Vec2(X(0.66f), headTop + h * 0.17f + open),
              Vec2(X(0.62f), headTop + h * 0.21f + open), Vec2(X(0.44f), headTop + h * 0.20f) },
            main.Scaled(0.8f), ink, alpha);
    if (jaw > 0.05f) {
        // 口の中と牙
        Poly({ Vec2(X(0.46f), headTop + h * 0.13f), Vec2(X(0.66f), headTop + h * 0.13f),
               Vec2(X(0.64f), headTop + h * 0.16f + open) }, ColorRGB(90, 20, 24), alpha);
        for (int i = 0; i < 3; ++i) {
            const float k = 0.50f + 0.05f * static_cast<float>(i);
            Poly({ Vec2(X(k), headTop + h * 0.125f), Vec2(X(k + 0.025f), headTop + h * 0.125f),
                   Vec2(X(k + 0.012f), headTop + h * 0.165f) }, palette::kWhite, alpha);
        }
    }
    // 頭と鼻先
    const std::vector<Vec2> skull = {
        Vec2(X(0.34f), headTop + h * 0.04f), Vec2(X(0.42f), headTop - h * 0.02f), Vec2(X(0.52f), headTop),
        Vec2(X(0.68f), headTop + h * 0.08f), Vec2(X(0.70f), headTop + h * 0.13f), Vec2(X(0.48f), headTop + h * 0.15f),
        Vec2(X(0.36f), headTop + h * 0.16f),
    };
    PolyInk(skull, main.Scaled(1.08f), ink, alpha);
    Poly({ Vec2(X(0.50f), headTop + h * 0.10f), Vec2(X(0.69f), headTop + h * 0.10f), Vec2(X(0.69f), headTop + h * 0.13f),
           Vec2(X(0.48f), headTop + h * 0.15f) }, belly, alpha);
    Ball(Vec2(X(0.69f), headTop + h * 0.09f), h * 0.022f, kInk, 0.0f, alpha);   // 鼻
    // 手前の耳
    PolyInk({ Vec2(X(0.38f), headTop + h * 0.02f), Vec2(X(0.46f), headTop), Vec2(X(0.40f), headTop - h * 0.15f) },
            main.Scaled(0.95f), ink * 0.7f, alpha);
    Poly({ Vec2(X(0.40f), headTop - h * 0.005f), Vec2(X(0.44f), headTop - h * 0.01f), Vec2(X(0.405f), headTop - h * 0.10f) },
         belly.Scaled(0.8f), alpha);
    // 目（攻撃時は光る）
    const bool angry = (pose == PoseKind::Attack || pose == PoseKind::Skill);
    const ColorRGB eye = angry ? art.trim : accent;
    const Vec2 eyeP(X(0.52f), headTop + h * 0.055f);
    if (angry) draw::Glow(eyeP.x, eyeP.y, h * 0.06f, art.trim, 140, 3);
    Poly({ Vec2(eyeP.x - dir * h * 0.03f, eyeP.y + h * 0.008f), Vec2(eyeP.x + dir * h * 0.03f, eyeP.y - h * 0.012f),
           Vec2(eyeP.x + dir * h * 0.01f, eyeP.y + h * 0.014f) }, eye, alpha);
    draw::Line(eyeP.x - dir * h * 0.035f, eyeP.y - h * 0.02f, eyeP.x + dir * h * 0.035f, eyeP.y - h * 0.03f, kInk,
               math::MaxF(1.0f, h * 0.012f), alpha);
    if (pose == PoseKind::Skill) draw::Glow(X(0.62f), headTop + h * 0.10f, h * 0.12f, art.trim, 120, 3);
}

//------------------------------------------------------------------------------
// 岩石系（ストーン・センチネル / ゴーレム・ガルド）
//   面取りした岩の塊を積み上げ、ひび・苔・光る紋様と胸の核を描く。
//------------------------------------------------------------------------------
void DrawGolem(const Rect& rect, int facing, PoseKind pose, float phase,
               const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent)
{
    const float w = rect.Width();
    const float h = rect.Height();
    const float cx = rect.CenterX();
    const float bottom = rect.bottom;
    const float dir = (facing >= 0) ? 1.0f : -1.0f;
    const float ink = math::MaxF(1.0f, h * 0.008f);
    (void)accent;

    const ColorRGB rock = main;
    const ColorRGB moss(78, 112, 70);
    const bool active = (pose == PoseKind::Skill || pose == PoseKind::Attack);
    const ColorRGB glow = active ? art.trim : art.trim.Scaled(0.75f);

    if (pose == PoseKind::Dead) {
        // 崩れた瓦礫
        for (int i = 0; i < 6; ++i) {
            const float bx = cx + (static_cast<float>(i) - 2.5f) * w * 0.19f;
            const float bh = h * (0.08f + 0.04f * static_cast<float>((i * 7) % 3));
            RockBlock(Rect::FromFoot(bx, bottom, w * 0.09f, bh), rock.Scaled(0.75f), ink, alpha);
        }
        draw::Glow(cx, bottom - h * 0.06f, h * 0.05f, art.trim, 80, 3);
        return;
    }

    float bob = 0.0f;
    float lunge = 0.0f;
    float stride = 0.0f;
    if (pose == PoseKind::Walk || pose == PoseKind::Run) {
        bob = std::fabs(std::sin(phase * kTwoPi)) * h * 0.04f;
        stride = std::sin(phase * kTwoPi) * w * 0.05f;
    } else if (active) {
        lunge = dir * w * 0.2f * std::sin(math::Clamp(phase, 0.0f, 1.0f) * math::kPi);
    } else if (pose == PoseKind::Idle) {
        bob = std::sin(phase * kTwoPi) * h * 0.012f;
    }

    // --- 奥の腕 -----------------------------------------------------------------
    const float torsoTop = bottom - h * 0.72f + bob;
    const float bodyX = cx + lunge * 0.3f;
    {
        const Vec2 s(bodyX - dir * w * 0.30f, torsoTop + h * 0.08f);
        const Vec2 hnd(s.x - dir * w * 0.06f, s.y + h * 0.30f);
        Capsule(s, hnd, h * 0.045f, rock.Scaled(0.6f), ink, alpha);
        RockBlock(Rect::FromCenter(hnd.x, hnd.y, w * 0.22f, h * 0.12f), rock.Scaled(0.62f), ink, alpha);
    }

    // --- 脚（太い柱と膝の岩）---------------------------------------------------------
    for (int i = 0; i < 2; ++i) {
        const float side = (i == 0) ? -1.0f : 1.0f;
        const float lx = cx + side * w * 0.20f + side * stride;
        const ColorRGB c = (i == 0) ? rock.Scaled(0.75f) : rock.Scaled(0.9f);
        RockBlock(Rect::FromFoot(lx, bottom - h * 0.14f + bob * 0.5f, w * 0.13f, h * 0.18f), c, ink, alpha);
        RockBlock(Rect::FromFoot(lx + dir * w * 0.02f, bottom, w * 0.17f, h * 0.14f), c.Scaled(0.9f), ink, alpha);
        RockBlock(Rect::FromCenter(lx, bottom - h * 0.16f + bob * 0.5f, w * 0.16f, h * 0.07f), c.Scaled(1.1f), ink,
                  alpha);
    }

    // --- 胴（肩の張った岩）------------------------------------------------------------
    const std::vector<Vec2> torso = {
        Vec2(bodyX - w * 0.44f, torsoTop + h * 0.05f), Vec2(bodyX - w * 0.20f, torsoTop - h * 0.03f),
        Vec2(bodyX + w * 0.22f, torsoTop - h * 0.02f), Vec2(bodyX + w * 0.46f, torsoTop + h * 0.06f),
        Vec2(bodyX + w * 0.34f, torsoTop + h * 0.40f), Vec2(bodyX + w * 0.20f, torsoTop + h * 0.50f),
        Vec2(bodyX - w * 0.22f, torsoTop + h * 0.50f), Vec2(bodyX - w * 0.36f, torsoTop + h * 0.38f),
    };
    PolyInk(torso, rock, ink, alpha);
    // 面（上を明るく、奥側を暗く）
    Poly({ torso[0], torso[1], torso[2], torso[3], Vec2(bodyX + w * 0.30f, torsoTop + h * 0.12f),
           Vec2(bodyX - w * 0.30f, torsoTop + h * 0.12f) }, rock.Scaled(1.25f), alpha);
    Poly({ Vec2(bodyX - dir * w * 0.44f, torsoTop + h * 0.05f), Vec2(bodyX - dir * w * 0.20f, torsoTop + h * 0.12f),
           Vec2(bodyX - dir * w * 0.12f, torsoTop + h * 0.50f), Vec2(bodyX - dir * w * 0.22f, torsoTop + h * 0.50f),
           Vec2(bodyX - dir * w * 0.36f, torsoTop + h * 0.38f) }, rock.Scaled(0.7f), alpha);
    // ひび
    const float crack = math::MaxF(1.0f, h * 0.006f);
    draw::Line(bodyX - w * 0.18f, torsoTop + h * 0.14f, bodyX - w * 0.08f, torsoTop + h * 0.26f, kInk, crack, alpha);
    draw::Line(bodyX - w * 0.08f, torsoTop + h * 0.26f, bodyX - w * 0.14f, torsoTop + h * 0.40f, kInk, crack, alpha);
    draw::Line(bodyX + w * 0.20f, torsoTop + h * 0.30f, bodyX + w * 0.28f, torsoTop + h * 0.42f, kInk, crack, alpha);
    // 苔
    for (int i = 0; i < 4; ++i) {
        const float mx = bodyX + w * (-0.30f + 0.17f * static_cast<float>(i));
        draw::Circle(mx, torsoTop + h * (0.015f + 0.01f * static_cast<float>(i % 2)), h * 0.022f, moss, true, 1.0f,
                     alpha);
    }
    // 光る紋様と胸の核
    const float coreY = torsoTop + h * 0.25f;
    const float rune = math::MaxF(1.0f, h * 0.008f);
    draw::Line(bodyX - w * 0.26f, coreY - h * 0.10f, bodyX - w * 0.12f, coreY - h * 0.02f, glow, rune, alpha);
    draw::Line(bodyX + w * 0.26f, coreY - h * 0.10f, bodyX + w * 0.12f, coreY - h * 0.02f, glow, rune, alpha);
    draw::Line(bodyX, coreY + h * 0.08f, bodyX, coreY + h * 0.20f, glow, rune, alpha);
    PolyInk({ Vec2(bodyX, coreY - h * 0.08f), Vec2(bodyX + w * 0.11f, coreY), Vec2(bodyX, coreY + h * 0.08f),
              Vec2(bodyX - w * 0.11f, coreY) },
            rock.Scaled(0.5f), ink, alpha);
    draw::Glow(bodyX, coreY, h * 0.09f, glow, 170, 4);
    draw::Circle(bodyX, coreY, h * 0.040f, glow, true, 1.0f, alpha);
    draw::Circle(bodyX - w * 0.01f, coreY - h * 0.01f, h * 0.020f, palette::kWhite, true, 1.0f, alpha);

    // --- 頭（肩に埋まった小さな岩と光る目）-------------------------------------------
    const Rect head = Rect::FromCenter(cx + dir * w * 0.06f + lunge * 0.4f, torsoTop - h * 0.06f, w * 0.26f, h * 0.15f);
    RockBlock(head, rock.Scaled(1.05f), ink, alpha);
    draw::FillRect(Rect(head.left - w * 0.01f, head.top + head.Height() * 0.30f, head.right + w * 0.01f,
                        head.top + head.Height() * 0.42f), rock.Scaled(0.6f), alpha);   // 眉の出っ張り
    const float eyeX = head.CenterX() + dir * head.Width() * 0.22f;
    const float eyeY = head.top + head.Height() * 0.58f;
    draw::Glow(eyeX, eyeY, h * 0.04f, glow, 160, 3);
    draw::Line(eyeX - dir * head.Width() * 0.18f, eyeY, eyeX + dir * head.Width() * 0.10f, eyeY, glow,
               math::MaxF(1.5f, h * 0.014f), alpha);

    // --- 肩の岩と手前の腕（攻撃で前へ）-------------------------------------------------
    const float armSwing = active ? math::Clamp(phase, 0.0f, 1.0f) : 0.0f;
    const Vec2 shoulder(cx + dir * w * 0.34f + lunge * 0.5f, torsoTop + h * 0.06f);
    const Vec2 hand(shoulder.x + dir * w * (0.10f + armSwing * 0.45f),
                    shoulder.y + h * (0.24f - armSwing * 0.22f));
    const Vec2 elbow = Joint(shoulder, hand, h * 0.15f, h * 0.15f, -dir);
    Capsule(shoulder, elbow, h * 0.05f, rock.Scaled(0.85f), ink, alpha);
    Capsule(elbow, hand, h * 0.045f, rock.Scaled(0.95f), ink, alpha);
    RockBlock(Rect::FromCenter(shoulder.x, shoulder.y - h * 0.02f, w * 0.26f, h * 0.12f), rock.Scaled(1.1f), ink, alpha);
    const Rect fist = Rect::FromCenter(hand.x, hand.y, w * 0.24f, h * 0.14f);
    RockBlock(fist, rock.Scaled(1.15f), ink, alpha);
    for (int i = 1; i <= 2; ++i) {   // 拳の指
        const float fy = fist.top + fist.Height() * 0.33f * static_cast<float>(i);
        draw::Line(fist.CenterX() - dir * fist.Width() * 0.05f, fy, fist.CenterX() + dir * fist.Width() * 0.45f, fy,
                   kInk, crack, alpha);
    }
}

//------------------------------------------------------------------------------
// 浮遊体（ダーク・ウィスプ）
//   揺らめく炎の体を外側から内側へ重ね、光る目と尾を引く火の粉を周回させる。
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
        const float fade = 1.0f - math::Clamp(phase, 0.0f, 1.0f);
        draw::Glow(cx, cy, w * 0.4f * fade, art.trim, static_cast<int>(120.0f * fade), 4);
        draw::Circle(cx, cy, w * 0.15f * fade, main, true, 1.0f, math::ClampInt(alpha / 2, 0, 255));
        return;
    }

    const bool angry = (pose == PoseKind::Attack || pose == PoseKind::Skill);
    draw::Glow(cx, cy, w * 0.62f, art.trim, angry ? 160 : 120, 5);

    // 炎の輪郭（上へ伸びる舌を 3 本、後ろへたなびかせる）
    auto flame = [&](float scale, const ColorRGB& color, float flicker) {
        const float r = w * 0.32f * scale;
        std::vector<Vec2> pts;
        const int n = 14;
        for (int i = 0; i < n; ++i) {
            const float a = kTwoPi * static_cast<float>(i) / static_cast<float>(n);
            float rr = r * (1.0f + 0.06f * std::sin(phase * kTwoPi * 2.0f + static_cast<float>(i) * 1.7f) * flicker);
            const float up = -std::sin(a);   // 上側ほど伸ばす
            if (up > 0.2f) rr *= 1.0f + 0.9f * (up - 0.2f) * (0.8f + 0.2f * std::sin(phase * kTwoPi * 3.0f + i));
            Vec2 p(cx + std::cos(a) * rr, cy - up * rr);
            p.x -= dir * (up > 0.0f ? up * r * 0.45f : 0.0f);   // 後ろへたなびく
            pts.push_back(p);
        }
        Poly(pts, color, alpha);
    };
    flame(1.12f, kInk, 0.0f);
    flame(1.0f, main, 1.0f);
    flame(0.72f, ColorRGB::Lerp(main, art.trim, 0.5f), 1.0f);
    flame(0.45f, ColorRGB::Lerp(art.trim, palette::kWhite, 0.3f), 1.0f);

    // 目
    for (int i = 0; i < 2; ++i) {
        const float ex = cx + dir * w * (0.06f + 0.12f * static_cast<float>(i));
        const float ey = cy - h * 0.02f;
        Poly({ Vec2(ex - w * 0.04f, ey - h * (angry ? 0.035f : 0.02f)), Vec2(ex + w * 0.04f, ey - h * 0.01f),
               Vec2(ex, ey + h * 0.03f) }, palette::kWhite, alpha);
    }
    draw::Circle(cx + dir * w * 0.12f, cy + h * 0.06f, w * 0.03f, kInk, true, 1.0f, alpha);   // 口

    // 周回する火の粉（尾を引く）
    for (int i = 0; i < 4; ++i) {
        const float a = phase * kTwoPi + static_cast<float>(i) * (kTwoPi / 4.0f);
        const Vec2 p(cx + std::cos(a) * w * 0.48f, cy + std::sin(a) * h * 0.24f);
        const Vec2 q(cx + std::cos(a - 0.5f) * w * 0.48f, cy + std::sin(a - 0.5f) * h * 0.24f);
        draw::Line(q.x, q.y, p.x, p.y, art.trim, w * 0.03f, alpha / 3);
        draw::Circle(p.x, p.y, w * 0.045f, accent, true, 1.0f, alpha);
        draw::Circle(p.x, p.y, w * 0.025f, palette::kWhite, true, 1.0f, alpha);
    }
}

//------------------------------------------------------------------------------
// 竜（竜王 / レッドドレイク / ドラゴンハッチリング）
//   羽ばたく翼、角のある頭と長い首、腹の鱗板、トゲの並んだ長い尾、爪のある脚。
//   攻撃中は口の奥に炎が灯る。
//------------------------------------------------------------------------------
void DrawDragon(const Rect& rect, int facing, PoseKind pose, float phase,
                const ActorArt& art, int alpha, const ColorRGB& main, const ColorRGB& accent)
{
    const float w = rect.Width();
    const float h = rect.Height();
    const float cx = rect.CenterX();
    const float bottom = rect.bottom;
    const float dir = (facing >= 0) ? 1.0f : -1.0f;
    const float ink = math::MaxF(1.0f, h * 0.010f);

    const ColorRGB scale = main;
    const ColorRGB dark = main.Scaled(0.6f);
    const ColorRGB belly = ColorRGB::Lerp(accent, main, 0.35f);
    const ColorRGB horn = ColorRGB::Lerp(accent, ColorRGB(240, 230, 210), 0.5f);
    const ColorRGB membrane = ColorRGB::Lerp(main.Scaled(0.75f), art.trim, 0.25f);

    if (pose == PoseKind::Dead) {
        const float fall = math::Clamp(phase, 0.0f, 1.0f);
        const float lieY = bottom - h * math::Lerp(0.40f, 0.14f, fall);
        PolyInk({ Vec2(cx - w * 0.60f, bottom - h * 0.02f), Vec2(cx - w * 0.20f, lieY - h * 0.10f),
                  Vec2(cx + w * 0.30f, lieY - h * 0.06f), Vec2(cx + w * 0.50f, bottom - h * 0.02f) },
                scale.Scaled(0.75f), ink, alpha);
        PolyInk({ Vec2(cx - w * 0.10f, lieY - h * 0.06f), Vec2(cx - w * 0.55f, lieY - h * 0.30f * (1.0f - fall)),
                  Vec2(cx + w * 0.20f, lieY - h * 0.04f) }, membrane.Scaled(0.7f), ink, alpha);
        Ball(Vec2(cx + dir * w * 0.62f, bottom - h * 0.08f), h * 0.09f, scale.Scaled(0.8f), ink, alpha);
        return;
    }

    float legSwing = 0.0f;
    float lunge = 0.0f;
    float jaw = 0.0f;
    float flapSpeed = 1.0f;
    const bool angry = (pose == PoseKind::Attack || pose == PoseKind::Skill);
    if (pose == PoseKind::Walk || pose == PoseKind::Run) {
        legSwing = std::sin(phase * kTwoPi) * h * 0.12f;
        flapSpeed = 1.5f;
    } else if (angry) {
        const float t = std::sin(math::Clamp(phase, 0.0f, 1.0f) * math::kPi);
        lunge = dir * w * 0.15f * t;
        jaw = t;
        flapSpeed = 2.0f;
    }
    const float flap = std::sin(phase * kTwoPi * flapSpeed);   // -1〜1
    const float top = bottom - h * 0.62f;                      // 背中の高さ
    const float under = bottom - h * 0.26f;                    // 腹の高さ
    auto X = [&](float k) { return cx + lunge + dir * w * k; };

    // --- 奥の翼 -----------------------------------------------------------------
    auto drawWing = [&](bool back) {
        const float lift = back ? 0.85f : 1.0f;
        const Vec2 root(X(back ? 0.02f : 0.08f), top + h * 0.06f);
        const Vec2 elbow(X(-0.10f), top - h * (0.30f + 0.12f * flap) * lift);
        const Vec2 tip(X(-0.55f), top - h * (0.38f + 0.22f * flap) * lift);
        const Vec2 f1(X(-0.62f), top - h * (0.05f + 0.10f * flap) * lift);
        const Vec2 f2(X(-0.42f), top + h * (0.10f - 0.04f * flap) * lift);
        const ColorRGB skin = back ? membrane.Scaled(0.65f) : membrane;
        const ColorRGB bone = back ? dark.Scaled(0.8f) : dark;
        PolyInk({ root, elbow, tip, f1, f2, Vec2(X(-0.18f), top + h * 0.12f) }, skin, ink, alpha);
        // 膜の筋
        for (const Vec2& f : { tip, f1, f2 }) {
            draw::Line(elbow.x, elbow.y, f.x, f.y, bone, math::MaxF(1.0f, h * 0.010f), alpha);
        }
        Capsule(root, elbow, h * 0.022f, bone, ink * 0.8f, alpha);
        Capsule(elbow, tip, h * 0.014f, bone, ink * 0.8f, alpha);
        PolyInk({ elbow + Vec2(-h * 0.015f, 0.0f), elbow + Vec2(h * 0.015f, 0.0f), elbow + Vec2(dir * h * 0.02f, -h * 0.06f) },
                horn, ink * 0.6f, alpha);
    };
    drawWing(true);

    // --- 奥の脚 -----------------------------------------------------------------
    auto drawLeg = [&](float baseK, float swingAmount, bool front, bool back) {
        const ColorRGB color = back ? dark.Scaled(0.8f) : scale.Scaled(0.92f);
        const Vec2 rootP(X(baseK), under - h * 0.06f);
        const Vec2 paw(X(baseK) + swingAmount, bottom - h * 0.03f);
        const float len = (paw.y - rootP.y) * 0.58f;
        const Vec2 knee = Joint(rootP, paw, len, len, front ? -dir : dir);
        Capsule(rootP, knee, h * (front ? 0.050f : 0.065f), color, ink, alpha);
        Capsule(knee, paw, h * 0.040f, color, ink, alpha);
        for (int i = 0; i < 3; ++i) {   // 爪
            const float tx = paw.x + dir * h * (0.00f + 0.035f * static_cast<float>(i));
            PolyInk({ Vec2(tx - dir * h * 0.012f, bottom - h * 0.03f), Vec2(tx + dir * h * 0.045f, bottom),
                      Vec2(tx - dir * h * 0.012f, bottom) }, horn, ink * 0.5f, alpha);
        }
    };
    drawLeg(0.20f, -legSwing, true, true);
    drawLeg(-0.24f, legSwing, false, true);

    // --- 尾（長く、先に向かって細くなり、背にトゲ）----------------------------------
    {
        const float wave = std::sin(phase * kTwoPi + 1.2f) * h * 0.05f;
        std::vector<Vec2> spine;
        for (int i = 0; i <= 5; ++i) {
            const float t = static_cast<float>(i) / 5.0f;
            spine.push_back(Vec2(X(-0.30f - 0.62f * t), top + h * (0.22f + 0.14f * t) + wave * t * t
                                  - h * 0.10f * t * t));
        }
        std::vector<Vec2> outline;
        for (size_t i = 0; i < spine.size(); ++i) {
            const float t = static_cast<float>(i) / 5.0f;
            outline.push_back(spine[i] + Vec2(0.0f, -h * 0.09f * (1.0f - t * 0.85f)));
        }
        for (size_t i = spine.size(); i-- > 0;) {
            const float t = static_cast<float>(i) / 5.0f;
            outline.push_back(spine[i] + Vec2(0.0f, h * 0.08f * (1.0f - t * 0.85f)));
        }
        // 尾は細長いので扇ではなく帯で塗る
        for (size_t i = 0; i + 1 < spine.size(); ++i) {
            const size_t n = outline.size();
            const Vec2 a = outline[i], b = outline[i + 1], c = outline[n - 2 - i], d = outline[n - 1 - i];
            Poly(Grow({ a, b, c, d }, ink), kInk, alpha);
        }
        for (size_t i = 0; i + 1 < spine.size(); ++i) {
            const size_t n = outline.size();
            Poly({ outline[i], outline[i + 1], outline[n - 2 - i], outline[n - 1 - i] }, scale.Scaled(0.9f), alpha);
            Poly({ spine[i], spine[i + 1], outline[n - 2 - i], outline[n - 1 - i] }, belly.Scaled(0.85f), alpha);
        }
        for (size_t i = 1; i < spine.size(); ++i) {
            const Vec2 base = outline[i];
            PolyInk({ base + Vec2(-h * 0.02f, 0.0f), base + Vec2(h * 0.02f, 0.0f),
                      base + Vec2(-dir * h * 0.03f, -h * 0.06f) }, art.trim.Scaled(0.85f), ink * 0.5f, alpha);
        }
        // 先端のひし形
        const Vec2 tipP = spine.back();
        PolyInk({ tipP + Vec2(dir * h * 0.02f, 0.0f), tipP + Vec2(-dir * h * 0.04f, -h * 0.05f),
                  tipP + Vec2(-dir * h * 0.10f, 0.0f), tipP + Vec2(-dir * h * 0.04f, h * 0.05f) },
                art.trim.Scaled(0.85f), ink * 0.7f, alpha);
    }

    // --- 胴 -----------------------------------------------------------------
    const std::vector<Vec2> body = {
        Vec2(X(-0.36f), top + h * 0.16f), Vec2(X(-0.18f), top + h * 0.02f), Vec2(X(0.12f), top - h * 0.02f),
        Vec2(X(0.30f), top + h * 0.06f), Vec2(X(0.34f), top + h * 0.24f), Vec2(X(0.22f), under + h * 0.02f),
        Vec2(X(-0.10f), under + h * 0.03f), Vec2(X(-0.34f), under - h * 0.02f), Vec2(X(-0.42f), top + h * 0.30f),
    };
    PolyInk(body, scale, ink, alpha);
    Poly({ Vec2(X(-0.34f), top + h * 0.14f), Vec2(X(-0.18f), top + h * 0.04f), Vec2(X(0.12f), top),
           Vec2(X(0.10f), top + h * 0.10f), Vec2(X(-0.30f), top + h * 0.18f) }, scale.Scaled(0.78f), alpha);
    // 腹の鱗板
    for (int i = 0; i < 6; ++i) {
        const float k = 0.24f - 0.09f * static_cast<float>(i);
        const float y = under - h * 0.02f - h * 0.006f * static_cast<float>(i);
        PolyInk({ Vec2(X(k + 0.04f), y - h * 0.06f), Vec2(X(k + 0.045f), y + h * 0.02f),
                  Vec2(X(k - 0.045f), y + h * 0.02f), Vec2(X(k - 0.04f), y - h * 0.06f) },
                belly, ink * 0.5f, alpha);
    }
    // 鱗の模様
    for (int row = 0; row < 2; ++row) {
        for (int i = 0; i < 5; ++i) {
            const float k = -0.26f + 0.11f * static_cast<float>(i) + 0.05f * static_cast<float>(row);
            const float y = top + h * (0.10f + 0.07f * static_cast<float>(row));
            draw::Circle(X(k), y, h * 0.022f, scale.Scaled(0.7f), false, math::MaxF(1.0f, h * 0.006f), alpha);
        }
    }
    // 背のトゲ
    for (int i = 0; i < 4; ++i) {
        const float k = -0.22f + 0.11f * static_cast<float>(i);
        const float y = top + h * (0.03f - 0.012f * static_cast<float>(i));
        PolyInk({ Vec2(X(k - 0.035f), y + h * 0.02f), Vec2(X(k + 0.035f), y + h * 0.01f), Vec2(X(k - 0.03f), y - h * 0.09f) },
                art.trim.Scaled(0.85f), ink * 0.6f, alpha);
    }

    // --- 手前の脚と翼 ------------------------------------------------------------
    drawLeg(0.26f, legSwing, true, false);
    drawLeg(-0.18f, -legSwing, false, false);
    drawWing(false);

    // --- 首（3 節で頭へ）と頭 -------------------------------------------------------
    const float rear = angry ? -0.02f * jaw : 0.0f;
    const Vec2 n0(X(0.28f), top + h * 0.12f);
    const Vec2 n1(X(0.38f + rear), top - h * 0.06f);
    const Vec2 n2(X(0.44f + rear), top - h * 0.20f);
    Capsule(n0, n1, h * 0.085f, scale, ink, alpha);
    Capsule(n1, n2, h * 0.070f, scale, ink, alpha);
    for (const Vec2& p : { n0 + (n1 - n0) * 0.5f, n1 + (n2 - n1) * 0.5f }) {   // 首の鱗板
        PolyInk({ p + Vec2(dir * h * 0.03f, -h * 0.02f), p + Vec2(dir * h * 0.08f, h * 0.0f),
                  p + Vec2(dir * h * 0.06f, h * 0.04f), p + Vec2(dir * h * 0.01f, h * 0.03f) },
                belly, ink * 0.5f, alpha);
    }

    const float hx = n2.x;
    const float hy = n2.y;
    auto H = [&](float kx, float ky) { return Vec2(hx + dir * h * kx, hy + h * ky); };
    // 角（奥・手前）
    PolyInk({ H(-0.02f, -0.05f), H(0.04f, -0.06f), H(-0.20f, -0.18f) }, horn.Scaled(0.75f), ink * 0.7f, alpha);
    // 下顎
    const float open = jaw * h * 0.07f;
    PolyInk({ H(0.00f, 0.04f), H(0.24f, 0.07f + open / h), H(0.22f, 0.10f + open / h), H(0.02f, 0.09f) },
            scale.Scaled(0.85f), ink, alpha);
    if (jaw > 0.05f) {
        Poly({ H(0.04f, 0.035f), H(0.24f, 0.04f), H(0.22f, 0.07f + open / h) }, ColorRGB(70, 14, 10), alpha);
        draw::Glow(hx + dir * h * 0.18f, hy + h * 0.06f, h * 0.08f * (0.6f + jaw), art.trim, 200, 4);
        for (int i = 0; i < 3; ++i) {
            const float k = 0.10f + 0.045f * static_cast<float>(i);
            Poly({ H(k, 0.03f), H(k + 0.02f, 0.03f), H(k + 0.01f, 0.065f) }, palette::kWhite, alpha);
        }
    }
    // 頭（長い鼻面）
    PolyInk({ H(-0.08f, -0.02f), H(-0.02f, -0.07f), H(0.10f, -0.06f), H(0.26f, 0.00f), H(0.27f, 0.04f),
              H(0.10f, 0.05f), H(-0.06f, 0.06f) },
            scale.Scaled(1.08f), ink, alpha);
    Poly({ H(0.02f, -0.055f), H(0.10f, -0.05f), H(0.24f, 0.00f), H(0.10f, -0.02f) }, scale.Scaled(1.3f), alpha);
    draw::Circle(hx + dir * h * 0.235f, hy - h * 0.005f, h * 0.010f, kInk, true, 1.0f, alpha);   // 鼻孔
    // 目（攻撃中は光る）と眉の隆起
    const Vec2 eye = H(0.07f, -0.025f);
    if (angry) draw::Glow(eye.x, eye.y, h * 0.05f, art.trim, 160, 3);
    Poly({ H(0.04f, -0.02f), H(0.11f, -0.035f), H(0.08f, -0.012f) }, angry ? art.trim : accent, alpha);
    draw::Line(hx + dir * h * 0.02f, hy - h * 0.045f, hx + dir * h * 0.12f, hy - h * 0.050f, kInk,
               math::MaxF(1.0f, h * 0.010f), alpha);
    // 手前の角と頬のトゲ
    PolyInk({ H(-0.04f, -0.04f), H(0.03f, -0.065f), H(-0.24f, -0.12f) }, horn, ink * 0.7f, alpha);
    PolyInk({ H(-0.06f, 0.03f), H(-0.02f, 0.05f), H(-0.14f, 0.08f) }, horn.Scaled(0.85f), ink * 0.5f, alpha);
    if (pose == PoseKind::Skill) draw::Glow(hx + dir * h * 0.20f, hy, h * 0.14f, art.trim, 140, 4);
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
    case ArtStyle::Dragon:
        DrawDragon(screenRect, facing, pose, phase, art, alpha, main, accent);
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
