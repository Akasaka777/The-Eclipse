#include "Game/Stage.h"

#include "Common/MathUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

namespace {

constexpr float kFarInf = 1.0e9f;

// 決定的な擬似乱数（背景の形状を毎フレーム同じにするため）
float Hash01(int seed)
{
    unsigned int x = static_cast<unsigned int>(seed) * 374761393u + 668265263u;
    x = (x ^ (x >> 13)) * 1274126177u;
    x = x ^ (x >> 16);
    return static_cast<float>(x % 10000u) / 10000.0f;
}

struct ThemeColors
{
    ColorRGB skyTop;
    ColorRGB skyBottom;
    ColorRGB far_;
    ColorRGB mid;
    ColorRGB ground;
    ColorRGB groundEdge;
    ColorRGB accent;
};

ThemeColors ColorsFor(StageTheme theme)
{
    ThemeColors c;
    switch (theme) {
    case StageTheme::Forest:
        c.skyTop = ColorRGB(14, 26, 40);
        c.skyBottom = ColorRGB(34, 62, 66);
        c.far_ = ColorRGB(20, 40, 44);
        c.mid = ColorRGB(16, 32, 34);
        c.ground = ColorRGB(38, 48, 40);
        c.groundEdge = ColorRGB(78, 122, 84);
        c.accent = ColorRGB(122, 224, 168);
        break;
    case StageTheme::Ruins:
        c.skyTop = ColorRGB(10, 12, 22);
        c.skyBottom = ColorRGB(44, 40, 56);
        c.far_ = ColorRGB(32, 30, 44);
        c.mid = ColorRGB(24, 22, 34);
        c.ground = ColorRGB(48, 46, 54);
        c.groundEdge = ColorRGB(120, 116, 132);
        c.accent = ColorRGB(255, 176, 96);
        break;
    case StageTheme::Altar:
        c.skyTop = ColorRGB(8, 6, 20);
        c.skyBottom = ColorRGB(42, 18, 54);
        c.far_ = ColorRGB(28, 16, 44);
        c.mid = ColorRGB(18, 10, 30);
        c.ground = ColorRGB(30, 22, 40);
        c.groundEdge = ColorRGB(150, 96, 220);
        c.accent = ColorRGB(206, 122, 255);
        break;
    case StageTheme::Volcano:
        c.skyTop = ColorRGB(22, 8, 8);
        c.skyBottom = ColorRGB(88, 26, 18);
        c.far_ = ColorRGB(52, 20, 18);
        c.mid = ColorRGB(34, 14, 14);
        c.ground = ColorRGB(38, 24, 22);
        c.groundEdge = ColorRGB(198, 86, 40);
        c.accent = ColorRGB(255, 142, 56);
        break;
    case StageTheme::Sanctum:
        // 金剛の伽藍：火山の奥に隠れた金色の寺院
        c.skyTop = ColorRGB(16, 10, 6);
        c.skyBottom = ColorRGB(74, 46, 18);
        c.far_ = ColorRGB(46, 30, 14);
        c.mid = ColorRGB(30, 19, 10);
        c.ground = ColorRGB(44, 32, 20);
        c.groundEdge = ColorRGB(222, 176, 86);
        c.accent = ColorRGB(255, 208, 112);
        break;
    case StageTheme::Throne:
        // 世界終焉の玉座：黒い太陽が昇る、崩れかけた世界の果て
        c.skyTop = ColorRGB(6, 4, 12);
        c.skyBottom = ColorRGB(58, 14, 34);
        c.far_ = ColorRGB(30, 14, 30);
        c.mid = ColorRGB(18, 9, 20);
        c.ground = ColorRGB(30, 22, 36);
        c.groundEdge = ColorRGB(226, 76, 118);
        c.accent = ColorRGB(255, 96, 128);
        break;
    case StageTheme::Home:
    default:
        c.skyTop = ColorRGB(10, 14, 30);
        c.skyBottom = ColorRGB(38, 46, 82);
        c.far_ = ColorRGB(26, 32, 56);
        c.mid = ColorRGB(20, 25, 44);
        c.ground = ColorRGB(40, 44, 62);
        c.groundEdge = ColorRGB(96, 132, 196);
        c.accent = ColorRGB(120, 190, 255);
        break;
    }
    return c;
}

} // namespace

void Stage::Load(const FloorDef& def)
{
    def_ = def;
    gateOpen_ = false;
    gateAnim_ = 0.0f;
    gateLabel_ = "▶ 次のフロアへ";
    time_ = 0.0f;
}

void Stage::Update(float dt)
{
    time_ += dt;
    const float target = gateOpen_ ? 1.0f : 0.0f;
    gateAnim_ = math::Approach(gateAnim_, target, dt * 2.0f);
}

void Stage::SetGateOpen(bool open)
{
    gateOpen_ = open;
}

bool Stage::ReachedGate(float x) const
{
    return gateOpen_ && x >= GateX() - 40.0f;
}

float Stage::ClampZ(float z) const
{
    return math::Clamp(z, 0.0f, def_.depth);
}

float Stage::LandingY(float x, float halfWidth, float prevBottom, float newBottom, float z) const
{
    float best = kFarInf;

    // 地面（奥にいるほど画面上では高い位置になる）
    const float ground = GroundYAt(z);
    if (newBottom >= ground) best = ground;

    // 足場（上からのみ乗れる。奥行き全体に伸びているものとして扱う）
    for (const Platform& platform : def_.platforms) {
        const float top = platform.rect.top - z;
        const float shrink = halfWidth * 0.5f;
        if (x + shrink < platform.rect.left || x - shrink > platform.rect.right) continue;
        if (prevBottom > top + 8.0f) continue;   // 既に足場より下にいる
        if (newBottom < top) continue;           // まだ到達していない
        if (top < best) best = top;
    }
    return best;
}

float Stage::ClampX(float x, float halfWidth) const
{
    return math::Clamp(x, halfWidth, def_.width - halfWidth);
}

//------------------------------------------------------------------------------
void Stage::DrawBackground(const Camera& camera) const
{
    DrawSky(camera);
    DrawFarLayer(camera);
    DrawMidLayer(camera);
    DrawDepthField(camera);
    DrawGround(camera);
    DrawPlatforms(camera);
}

void Stage::DrawForeground(const Camera& camera) const
{
    DrawGate(camera);
}

void Stage::DrawSky(const Camera& camera) const
{
    const ThemeColors colors = ColorsFor(def_.theme);
    const Rect screen(0.0f, 0.0f, static_cast<float>(config::kScreenWidth),
                      static_cast<float>(config::kScreenHeight));
    draw::GradientRectV(screen, colors.skyTop, colors.skyBottom, 255, 40);

    // 星
    const float parallax = camera.ViewLeft() * 0.05f;
    for (int i = 0; i < 90; ++i) {
        const float x = std::fmod(Hash01(i * 3 + 1) * 3000.0f - parallax, 2100.0f);
        const float y = Hash01(i * 3 + 2) * 520.0f;
        const float twinkle = 0.55f + 0.45f * std::sin(time_ * 1.6f + static_cast<float>(i));
        draw::Circle(x, y, 1.0f + Hash01(i * 3 + 3) * 1.6f, palette::kWhite, true, 1.0f,
                     static_cast<int>(150.0f * twinkle));
    }

    // 天体（蝕の祭壇では日蝕）
    const float moonX = 1520.0f - camera.ViewLeft() * 0.04f;
    const float moonY = 210.0f;
    if (def_.theme == StageTheme::Sanctum) {
        // 天に浮かぶ大きな光背（ゆっくり回る）
        const ThemeColors gold = ColorsFor(def_.theme);
        // ボスの光背と重なっても主役が埋もれないよう、空の光背は控えめにする
        draw::Glow(moonX, moonY, 230.0f, gold.accent, 70, 6);
        draw::Circle(moonX, moonY, 150.0f, gold.accent, false, 6.0f, 110);
        draw::Circle(moonX, moonY, 128.0f, gold.accent, false, 2.0f, 80);
        for (int i = 0; i < 24; ++i) {
            const float a = time_ * 0.15f + static_cast<float>(i) * (6.2831853f / 24.0f);
            const float c = std::cos(a), s = std::sin(a);
            draw::Triangle(Vec2(moonX + c * 156.0f - s * 9.0f, moonY + s * 156.0f + c * 9.0f),
                           Vec2(moonX + c * 156.0f + s * 9.0f, moonY + s * 156.0f - c * 9.0f),
                           Vec2(moonX + c * 190.0f, moonY + s * 190.0f), gold.accent, true, 90);
        }
        draw::Circle(moonX, moonY, 40.0f, ColorRGB(255, 236, 190), true, 1.0f, 150);
    } else if (def_.theme == StageTheme::Throne) {
        // 黒い太陽：赤いコロナと、ゆっくり回る欠けた輪。空には裂け目が走る
        const ThemeColors end = ColorsFor(def_.theme);
        for (int i = 0; i < 5; ++i) {
            const float sx = std::fmod(Hash01(i * 13 + 5) * 2400.0f - camera.ViewLeft() * 0.03f, 2200.0f) - 140.0f;
            const float sy = 60.0f + Hash01(i * 13 + 6) * 360.0f;
            float px = sx, py = sy;
            for (int k = 0; k < 5; ++k) {
                const float nx = px + 40.0f + Hash01(i * 13 + k * 3 + 7) * 70.0f;
                const float ny = py + (Hash01(i * 13 + k * 3 + 8) - 0.5f) * 80.0f;
                draw::Line(px, py, nx, ny, end.accent, 2.0f, 70);
                draw::Line(px, py, nx, ny, ColorRGB(255, 220, 230), 1.0f, 60);
                px = nx;
                py = ny;
            }
        }
        draw::Glow(moonX, moonY, 260.0f, end.accent, 120, 6);
        draw::Circle(moonX, moonY, 112.0f, ColorRGB(255, 150, 120), true, 1.0f, 200);
        draw::Circle(moonX, moonY, 100.0f, ColorRGB(4, 2, 8), true, 1.0f, 255);
        for (int i = 0; i < 16; ++i) {
            if (i % 4 == 3) continue;
            const float a0 = -time_ * 0.10f + static_cast<float>(i) * (6.2831853f / 16.0f);
            const float a1 = a0 + 6.2831853f / 16.0f * 0.8f;
            draw::Line(moonX + std::cos(a0) * 150.0f, moonY + std::sin(a0) * 150.0f,
                       moonX + std::cos(a1) * 150.0f, moonY + std::sin(a1) * 150.0f, end.accent, 3.0f, 120);
        }
    } else if (def_.theme == StageTheme::Altar) {
        draw::Glow(moonX, moonY, 200.0f, ColorRGB(210, 120, 255), 150, 6);
        draw::Circle(moonX, moonY, 96.0f, ColorRGB(255, 226, 200), true, 1.0f, 255);
        draw::Circle(moonX, moonY, 88.0f, ColorRGB(10, 6, 16), true, 1.0f, 255);
    } else {
        draw::Glow(moonX, moonY, 150.0f, ColorRGB(180, 210, 255), 90, 5);
        draw::Circle(moonX, moonY, 78.0f, ColorRGB(232, 240, 255), true, 1.0f, 235);
        draw::Circle(moonX - 24.0f, moonY - 16.0f, 14.0f, ColorRGB(206, 216, 236), true, 1.0f, 200);
        draw::Circle(moonX + 18.0f, moonY + 22.0f, 20.0f, ColorRGB(206, 216, 236), true, 1.0f, 180);
    }
}

void Stage::DrawFarLayer(const Camera& camera) const
{
    const ThemeColors colors = ColorsFor(def_.theme);
    const float parallax = camera.ViewLeft() * 0.25f;
    const float baseY = def_.groundY + 40.0f;

    for (int i = 0; i < 26; ++i) {
        const float seedX = Hash01(i * 7 + 11);
        const float x = std::fmod(seedX * 3600.0f + static_cast<float>(i) * 180.0f - parallax, 2400.0f) - 240.0f;
        const float h = 260.0f + Hash01(i * 7 + 12) * 300.0f;
        const float w = 130.0f + Hash01(i * 7 + 13) * 180.0f;

        switch (def_.theme) {
        case StageTheme::Forest:
            // 遠景の木
            draw::Triangle(Vec2(x - w * 0.5f, baseY), Vec2(x + w * 0.5f, baseY), Vec2(x, baseY - h),
                           colors.far_, true, 255);
            break;
        case StageTheme::Ruins:
            // 崩れた尖塔
            draw::FillRect(Rect(x - w * 0.3f, baseY - h, x + w * 0.3f, baseY), colors.far_, 255);
            draw::Triangle(Vec2(x - w * 0.3f, baseY - h), Vec2(x + w * 0.3f, baseY - h),
                           Vec2(x, baseY - h - 60.0f), colors.far_, true, 255);
            break;
        case StageTheme::Altar:
            // 浮遊する岩塊
            draw::FillRect(Rect(x - w * 0.4f, baseY - h, x + w * 0.4f, baseY - h + 60.0f), colors.far_, 255);
            draw::Triangle(Vec2(x - w * 0.4f, baseY - h + 60.0f), Vec2(x + w * 0.4f, baseY - h + 60.0f),
                           Vec2(x, baseY - h + 160.0f), colors.far_, true, 255);
            break;
        case StageTheme::Volcano:
            // 噴煙を上げる火山
            draw::Triangle(Vec2(x - w * 0.7f, baseY), Vec2(x + w * 0.7f, baseY),
                           Vec2(x, baseY - h), colors.far_, true, 255);
            draw::Glow(x, baseY - h + 20.0f, 54.0f, colors.accent, 90, 4);
            break;
        case StageTheme::Sanctum: {
            // 層塔（屋根を重ねた塔）の影
            const int tiers = 3 + static_cast<int>(Hash01(i * 7 + 14) * 3.0f);
            const float tierH = h / static_cast<float>(tiers + 1);
            for (int t = 0; t < tiers; ++t) {
                const float y = baseY - tierH * static_cast<float>(t + 1);
                const float roofW = w * (0.62f - 0.07f * static_cast<float>(t));
                draw::FillRect(Rect(x - roofW * 0.32f, y, x + roofW * 0.32f, y + tierH), colors.far_, 255);
                draw::Triangle(Vec2(x - roofW * 0.62f, y + tierH * 0.18f), Vec2(x + roofW * 0.62f, y + tierH * 0.18f),
                               Vec2(x, y - tierH * 0.42f), colors.far_, true, 255);
            }
            draw::Line(x, baseY - tierH * static_cast<float>(tiers + 1), x, baseY - h - 30.0f, colors.far_, 4.0f, 255);
            break;
        }
        case StageTheme::Throne: {
            // 宙に浮かぶ、砕けた大地のかけら（下側は尖った岩、縁が赤く光る）
            const float top = baseY - h - 40.0f + std::sin(time_ * 0.4f + static_cast<float>(i)) * 8.0f;
            draw::FillRect(Rect(x - w * 0.45f, top, x + w * 0.45f, top + 34.0f), colors.far_, 255);
            draw::Triangle(Vec2(x - w * 0.45f, top + 34.0f), Vec2(x + w * 0.45f, top + 34.0f),
                           Vec2(x + w * 0.08f, top + 34.0f + h * 0.45f), colors.far_, true, 255);
            draw::Line(x - w * 0.45f, top, x + w * 0.45f, top, colors.accent, 2.0f, 90);
            // ところどころ崩れた塔が乗っている
            if (Hash01(i * 7 + 15) > 0.5f) {
                draw::FillRect(Rect(x - w * 0.10f, top - h * 0.30f, x + w * 0.06f, top), colors.far_, 255);
                draw::Triangle(Vec2(x - w * 0.10f, top - h * 0.30f), Vec2(x + w * 0.06f, top - h * 0.30f),
                               Vec2(x + w * 0.10f, top - h * 0.38f), colors.far_, true, 255);
            }
            break;
        }
        case StageTheme::Home:
        default:
            // 遠景の街並み
            draw::FillRect(Rect(x - w * 0.5f, baseY - h * 0.6f, x + w * 0.5f, baseY), colors.far_, 255);
            draw::Triangle(Vec2(x - w * 0.55f, baseY - h * 0.6f), Vec2(x + w * 0.55f, baseY - h * 0.6f),
                           Vec2(x, baseY - h * 0.85f), colors.far_, true, 255);
            break;
        }
    }
}

void Stage::DrawMidLayer(const Camera& camera) const
{
    const ThemeColors colors = ColorsFor(def_.theme);
    const float parallax = camera.ViewLeft() * 0.55f;
    const float baseY = def_.groundY + 10.0f;

    for (int i = 0; i < 20; ++i) {
        const float x = std::fmod(Hash01(i * 5 + 31) * 2600.0f + static_cast<float>(i) * 240.0f - parallax,
                                  2400.0f) - 240.0f;
        const float h = 180.0f + Hash01(i * 5 + 32) * 220.0f;
        const float w = 60.0f + Hash01(i * 5 + 33) * 80.0f;

        switch (def_.theme) {
        case StageTheme::Forest: {
            // 幹と葉
            draw::FillRect(Rect(x - w * 0.12f, baseY - h, x + w * 0.12f, baseY), colors.mid, 255);
            draw::Circle(x, baseY - h, w * 0.75f, colors.mid, true, 1.0f, 255);
            draw::Circle(x - w * 0.5f, baseY - h + 30.0f, w * 0.5f, colors.mid, true, 1.0f, 255);
            draw::Circle(x + w * 0.5f, baseY - h + 20.0f, w * 0.55f, colors.mid, true, 1.0f, 255);
            break;
        }
        case StageTheme::Ruins: {
            // 石柱と松明
            draw::FillRect(Rect(x - w * 0.3f, baseY - h, x + w * 0.3f, baseY), colors.mid, 255);
            draw::FillRect(Rect(x - w * 0.42f, baseY - h - 18.0f, x + w * 0.42f, baseY - h), colors.mid, 255);
            const float flicker = 0.75f + 0.25f * std::sin(time_ * 9.0f + static_cast<float>(i) * 2.1f);
            draw::Glow(x, baseY - h - 40.0f, 52.0f * flicker, colors.accent, 130, 4);
            break;
        }
        case StageTheme::Altar: {
            // オベリスク
            draw::FillRect(Rect(x - w * 0.22f, baseY - h, x + w * 0.22f, baseY), colors.mid, 255);
            draw::Triangle(Vec2(x - w * 0.22f, baseY - h), Vec2(x + w * 0.22f, baseY - h),
                           Vec2(x, baseY - h - 70.0f), colors.mid, true, 255);
            const float pulse = 0.6f + 0.4f * std::sin(time_ * 2.2f + static_cast<float>(i));
            draw::Circle(x, baseY - h * 0.55f, 10.0f, colors.accent, true, 1.0f,
                         static_cast<int>(200.0f * pulse));
            break;
        }
        case StageTheme::Sanctum: {
            // 丸柱と、吊るした灯籠
            draw::FillRect(Rect(x - w * 0.16f, baseY - h, x + w * 0.16f, baseY), colors.mid, 255);
            draw::FillRect(Rect(x - w * 0.30f, baseY - h - 14.0f, x + w * 0.30f, baseY - h), colors.mid, 255);
            const float sway = std::sin(time_ * 1.3f + static_cast<float>(i)) * 4.0f;
            const float lanternY = baseY - h * 0.62f;
            draw::Line(x + w * 0.16f, baseY - h + 20.0f, x + w * 0.42f + sway, lanternY - 16.0f, colors.mid, 2.0f, 255);
            const float flicker = 0.75f + 0.25f * std::sin(time_ * 6.0f + static_cast<float>(i) * 1.9f);
            draw::Glow(x + w * 0.42f + sway, lanternY, 40.0f * flicker, colors.accent, 120, 4);
            draw::FillRect(Rect::FromCenter(x + w * 0.42f + sway, lanternY, 14.0f, 20.0f), colors.accent.Scaled(0.8f), 230);
            break;
        }
        case StageTheme::Volcano: {
            // 固まった溶岩の柱と、噴き出す炎
            draw::Triangle(Vec2(x - w * 0.45f, baseY), Vec2(x + w * 0.45f, baseY),
                           Vec2(x, baseY - h), colors.mid, true, 255);
            const float burst = 0.6f + 0.4f * std::sin(time_ * 3.4f + static_cast<float>(i) * 1.7f);
            draw::Glow(x, baseY - h * 0.35f, 46.0f * burst, colors.accent, 120, 4);
            break;
        }
        case StageTheme::Throne: {
            // 折れた柱と、根元に突き刺さった赤い結晶
            const float broken = h * (0.55f + Hash01(i * 5 + 34) * 0.35f);
            draw::FillRect(Rect(x - w * 0.18f, baseY - broken, x + w * 0.18f, baseY), colors.mid, 255);
            draw::Triangle(Vec2(x - w * 0.18f, baseY - broken), Vec2(x + w * 0.18f, baseY - broken),
                           Vec2(x + w * 0.18f, baseY - broken - 34.0f), colors.mid, true, 255);
            draw::FillRect(Rect(x - w * 0.26f, baseY - 16.0f, x + w * 0.26f, baseY), colors.mid, 255);
            const float pulse = 0.6f + 0.4f * std::sin(time_ * 1.8f + static_cast<float>(i) * 1.3f);
            const float cxr = x + w * 0.34f;
            draw::Glow(cxr, baseY - 34.0f, 46.0f * pulse, colors.accent, 110, 4);
            draw::Triangle(Vec2(cxr - 10.0f, baseY), Vec2(cxr + 10.0f, baseY), Vec2(cxr + 4.0f, baseY - 70.0f),
                           colors.accent.Scaled(0.85f), true, 220);
            break;
        }
        case StageTheme::Home:
        default: {
            // 街の奥の家並み（三角屋根の影と、明かりの灯った窓）
            const float houseH = h * 0.62f;
            const float houseW = w * 1.3f;
            draw::FillRect(Rect(x - houseW * 0.5f, baseY - houseH, x + houseW * 0.5f, baseY), colors.mid, 255);
            draw::Triangle(Vec2(x - houseW * 0.62f, baseY - houseH), Vec2(x + houseW * 0.62f, baseY - houseH),
                           Vec2(x, baseY - houseH - houseW * 0.45f), colors.mid, true, 255);
            const float flicker = 0.75f + 0.25f * std::sin(time_ * 1.3f + static_cast<float>(i) * 2.3f);
            for (int k = 0; k < 2; ++k) {
                if (Hash01(i * 5 + 35 + k) < 0.35f) continue;
                const float wx = x + houseW * (k == 0 ? -0.22f : 0.22f);
                const float wy = baseY - houseH * 0.62f;
                draw::FillRect(Rect::FromCenter(wx, wy, 16.0f, 20.0f), ColorRGB(255, 200, 120),
                               static_cast<int>(170.0f * flicker));
            }
            break;
        }
        }
    }
}

void Stage::DrawDepthField(const Camera& camera) const
{
    if (def_.depth <= 0.0f) return;

    const ThemeColors colors = ColorsFor(def_.theme);
    const float nearY = def_.groundY - camera.ViewTop();
    const float farY = nearY - def_.depth;
    const float screenW = static_cast<float>(config::kScreenWidth);

    // 奥（暗い）→ 手前（明るい）のグラデーションで歩行可能な床を示す
    draw::GradientRectV(Rect(0.0f, farY, screenW, nearY),
                        colors.ground.Scaled(0.55f), colors.ground.Scaled(1.05f), 255, 18);

    // 奥行きの目安になる横線
    const int lines = 5;
    for (int i = 1; i < lines; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(lines);
        const float y = nearY - def_.depth * t;
        draw::Line(0.0f, y, screenW, y, colors.groundEdge, 1.0f, 40);
    }

    // 最奥の縁
    draw::Line(0.0f, farY, screenW, farY, colors.groundEdge, 2.0f, 150);

    // 奥行きを感じさせる縦のガイド（手前ほど間隔が広い）
    const float viewLeft = camera.ViewLeft();
    const float tile = 220.0f;
    const float offset = std::fmod(viewLeft, tile);
    for (int i = 0; i <= static_cast<int>(screenW / tile) + 1; ++i) {
        const float x = static_cast<float>(i) * tile - offset;
        draw::Line(x, nearY, x + 26.0f, farY, colors.groundEdge, 1.0f, 28);
    }
}

void Stage::DrawGround(const Camera& camera) const
{
    const ThemeColors colors = ColorsFor(def_.theme);
    const float screenGroundY = def_.groundY - camera.ViewTop();

    const Rect ground(0.0f, screenGroundY, static_cast<float>(config::kScreenWidth),
                      static_cast<float>(config::kScreenHeight));
    draw::GradientRectV(ground, colors.ground, colors.ground.Scaled(0.45f), 255, 16);

    // 地表のライン
    draw::Line(0.0f, screenGroundY, static_cast<float>(config::kScreenWidth), screenGroundY,
               colors.groundEdge, 3.0f, 255);

    // タイル目地
    const float viewLeft = camera.ViewLeft();
    const float tile = 160.0f;
    const float offset = std::fmod(viewLeft, tile);
    for (int i = 0; i <= config::kScreenWidth / static_cast<int>(tile) + 1; ++i) {
        const float x = static_cast<float>(i) * tile - offset;
        draw::Line(x, screenGroundY, x, screenGroundY + 46.0f, colors.groundEdge, 1.0f, 70);
    }
    // 溶岩の裂け目
    if (def_.theme == StageTheme::Volcano) {
        for (int i = 0; i < 70; ++i) {
            const float worldX = Hash01(i * 13 + 5) * def_.width;
            const float x = worldX - viewLeft;
            if (x < -60.0f || x > static_cast<float>(config::kScreenWidth) + 60.0f) continue;
            const float w = 24.0f + Hash01(i * 13 + 6) * 60.0f;
            const float glow = 0.55f + 0.45f * std::sin(time_ * 2.0f + static_cast<float>(i) * 0.9f);
            const float y = screenGroundY + 10.0f + Hash01(i * 13 + 7) * 30.0f;
            draw::Line(x, y, x + w, y, colors.accent, 3.0f, static_cast<int>(170.0f * glow));
        }
    }

    // 草／苔の表現
    if (def_.theme == StageTheme::Forest) {
        for (int i = 0; i < 80; ++i) {
            const float worldX = Hash01(i * 11 + 3) * def_.width;
            const float x = worldX - viewLeft;
            if (x < -20.0f || x > static_cast<float>(config::kScreenWidth) + 20.0f) continue;
            const float h = 10.0f + Hash01(i * 11 + 4) * 16.0f;
            draw::Line(x, screenGroundY, x + 4.0f, screenGroundY - h, colors.accent, 2.0f, 110);
        }
    }
}

void Stage::DrawPlatforms(const Camera& camera) const
{
    const ThemeColors colors = ColorsFor(def_.theme);

    for (const Platform& platform : def_.platforms) {
        if (!camera.IsVisible(platform.rect.Offset(0.0f, -def_.depth).Expanded(def_.depth))) continue;
        const Rect screen = camera.WorldToScreen(platform.rect);

        // 上面（奥行き方向へ伸びる面）
        const float top = screen.top;
        const float farTop = top - def_.depth;
        draw::GradientRectV(Rect(screen.left, farTop, screen.right, top),
                            colors.ground.Scaled(0.85f), colors.ground.Scaled(1.45f), 255, 10);
        draw::Line(screen.left, farTop, screen.right, farTop, colors.groundEdge, 2.0f, 200);

        // 手前の側面
        draw::GradientRectV(screen, colors.ground.Scaled(1.15f), colors.ground.Scaled(0.55f), 255, 8);
        draw::Line(screen.left, top, screen.right, top, colors.groundEdge, 3.0f, 255);
        draw::StrokeRect(screen, colors.ground.Scaled(0.5f), 1.0f, 200);
    }
}

void Stage::DrawGate(const Camera& camera) const
{
    if (def_.theme == StageTheme::Home) return;
    // ボスフロアには閉じたゲートを置かない（ボスを倒して開いたとき＝隠しフロア・ボスラッシュの次のフロアへ進むときだけ出す）
    if (IsBossFloor() && !gateOpen_ && gateAnim_ <= 0.01f) return;

    const float gateWorldX = GateX();
    const Rect area = Rect::FromCenter(gateWorldX, def_.groundY - 200.0f, 200.0f, 400.0f);
    if (!camera.IsVisible(area, 200.0f)) return;

    const Rect screen = camera.WorldToScreen(area);
    const ThemeColors colors = ColorsFor(def_.theme);

    if (gateAnim_ > 0.01f) {
        // 開放：光の柱
        const int alpha = static_cast<int>(180.0f * gateAnim_);
        const float pulse = 0.85f + 0.15f * std::sin(time_ * 4.0f);
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
        for (int i = 0; i < 5; ++i) {
            const float w = screen.Width() * (0.2f + 0.16f * static_cast<float>(i)) * pulse;
            DrawBoxAA(screen.CenterX() - w * 0.5f, screen.top, screen.CenterX() + w * 0.5f, screen.bottom,
                      draw::ToDx(palette::kAccent), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 上昇する粒子
        for (int i = 0; i < 14; ++i) {
            const float t = std::fmod(time_ * 0.6f + Hash01(i * 13 + 7), 1.0f);
            const float px = screen.CenterX() + (Hash01(i * 13 + 8) - 0.5f) * screen.Width() * 0.8f;
            const float py = screen.bottom - t * screen.Height();
            draw::Circle(px, py, 3.0f + Hash01(i * 13 + 9) * 3.0f, palette::kAccent, true, 1.0f,
                         static_cast<int>((1.0f - t) * 220.0f * gateAnim_));
        }
        draw::Text(FontSize::Medium, screen.CenterX(), screen.top - 50.0f, palette::kAccent,
                   gateLabel_, draw::TextAlign::Center);
    } else {
        // 封鎖：障壁
        const int alpha = 150;
        draw::FillRect(screen, colors.accent.Scaled(0.35f), alpha);
        draw::StrokeRect(screen, colors.accent, 3.0f, 220);
        for (int i = 0; i < 6; ++i) {
            const float y = screen.top + screen.Height() * (static_cast<float>(i) / 6.0f)
                          + std::fmod(time_ * 40.0f, screen.Height() / 6.0f);
            draw::Line(screen.left, y, screen.right, y, colors.accent, 2.0f, 120);
        }
        draw::Text(FontSize::Small, screen.CenterX(), screen.top - 40.0f, palette::kTextDim,
                   "敵を全て倒すと開放", draw::TextAlign::Center);
    }
}

} // namespace ecl
