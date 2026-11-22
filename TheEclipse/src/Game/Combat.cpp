#include "Game/Combat.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <algorithm>
#include <cmath>

namespace ecl {

namespace {
constexpr float kTwoPi = 6.28318530718f;
} // namespace

bool HitBox::AlreadyHit(int targetId) const
{
    return std::find(hitTargets.begin(), hitTargets.end(), targetId) != hitTargets.end();
}

void HitBox::MarkHit(int targetId)
{
    hitTargets.push_back(targetId);
}

void CombatSystem::Clear()
{
    hitBoxes_.clear();
    projectiles_.clear();
    damageNumbers_.clear();
    particles_.clear();
    slashes_.clear();
    rings_.clear();
    popups_.clear();
    bolts_.clear();
}

void CombatSystem::AddHitBox(const HitBox& hitBox)
{
    hitBoxes_.push_back(hitBox);
}

void CombatSystem::MoveHitBoxes(int sourceId, const Rect& area, float z)
{
    // 同じ発生元の持続判定を、今の位置へ動かす
    for (HitBox& hitBox : hitBoxes_) {
        if (hitBox.sourceId != sourceId) continue;
        hitBox.area = area;
        hitBox.z = z;
    }
}

void CombatSystem::AddProjectile(const Projectile& projectile)
{
    projectiles_.push_back(projectile);
}

void CombatSystem::AddDamageNumber(const Vec2& pos, int value, bool critical, const ColorRGB& color)
{
    DamageNumber number;
    number.pos = pos;
    number.velocity = Vec2(math::RandFloat(-70.0f, 70.0f), math::RandFloat(-420.0f, -320.0f));
    number.value = value;
    number.critical = critical;
    number.color = color;
    number.maxLife = critical ? 1.1f : 0.85f;
    damageNumbers_.push_back(number);
}

void CombatSystem::AddHealNumber(const Vec2& pos, int value)
{
    DamageNumber number;
    number.pos = pos;
    number.velocity = Vec2(0.0f, -220.0f);
    number.value = -value; // 負値は回復として描画
    number.color = palette::kHp;
    number.maxLife = 1.0f;
    damageNumbers_.push_back(number);
}

void CombatSystem::AddSlash(const Vec2& pos, int facing, float radius, const ColorRGB& color, int style)
{
    SlashEffect slash;
    slash.pos = pos;
    slash.facing = facing;
    slash.radius = radius;
    slash.color = color;
    slash.style = style;
    slash.maxLife = (style == 3) ? 0.16f : 0.22f;
    slash.angle = math::RandFloat(-0.25f, 0.25f);
    slashes_.push_back(slash);
}

void CombatSystem::AddImpact(const Vec2& pos, const ColorRGB& color, int count, float speed)
{
    for (int i = 0; i < count; ++i) {
        Particle p;
        const float angle = math::RandFloat(0.0f, kTwoPi);
        const float power = math::RandFloat(speed * 0.35f, speed);
        p.pos = pos;
        p.velocity = Vec2(std::cos(angle) * power, std::sin(angle) * power);
        p.maxLife = math::RandFloat(0.18f, 0.42f);
        p.size = math::RandFloat(3.0f, 8.0f);
        p.gravity = 900.0f;
        p.color = color;
        p.kind = (i % 3 == 0) ? 1 : 0;
        particles_.push_back(p);
    }
}

void CombatSystem::AddDust(const Vec2& pos, int facing, int count)
{
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = Vec2(pos.x + math::RandFloat(-10.0f, 10.0f), pos.y);
        p.velocity = Vec2(-static_cast<float>(facing) * math::RandFloat(60.0f, 220.0f),
                          math::RandFloat(-180.0f, -40.0f));
        p.maxLife = math::RandFloat(0.25f, 0.5f);
        p.size = math::RandFloat(4.0f, 9.0f);
        p.gravity = 500.0f;
        p.color = ColorRGB(170, 165, 150);
        p.kind = 1;
        particles_.push_back(p);
    }
}

void CombatSystem::AddRing(const Vec2& pos, float maxRadius, const ColorRGB& color, float duration)
{
    RingEffect ring;
    ring.pos = pos;
    ring.maxRadius = maxRadius;
    ring.maxLife = duration;
    ring.color = color;
    rings_.push_back(ring);
}

void CombatSystem::AddPopup(const Vec2& pos, const std::string& text, const ColorRGB& color, bool large)
{
    PopupText popup;
    popup.pos = pos;
    popup.text = text;
    popup.color = color;
    popup.large = large;
    popups_.push_back(popup);
}

void CombatSystem::AddBolt(float x, float topY, float bottomY, const ColorRGB& color, float duration)
{
    BoltEffect bolt;
    bolt.x = x;
    bolt.topY = topY;
    bolt.bottomY = bottomY;
    bolt.maxLife = duration;
    bolt.seed = math::RandInt(0, 100000);
    bolt.color = color;
    bolts_.push_back(bolt);
}

void CombatSystem::Update(float dt)
{
    // --- 攻撃判定 -----------------------------------------------------------
    for (HitBox& hitBox : hitBoxes_) {
        hitBox.timer += dt;
        if (hitBox.multiHit && hitBox.timer >= hitBox.hitInterval) {
            hitBox.timer = 0.0f;
            hitBox.hitTargets.clear();
        }
        hitBox.life -= dt;
    }
    hitBoxes_.erase(std::remove_if(hitBoxes_.begin(), hitBoxes_.end(),
                                   [](const HitBox& h) { return h.life <= 0.0f; }),
                    hitBoxes_.end());

    // --- 飛び道具 -----------------------------------------------------------
    for (Projectile& projectile : projectiles_) {
        if (projectile.useGravity) projectile.velocity.y += config::kGravity * 0.45f * dt;
        projectile.pos += projectile.velocity * dt;
        projectile.life -= dt;
        if (projectile.life <= 0.0f) projectile.active = false;
    }
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
                                      [](const Projectile& p) { return !p.active; }),
                       projectiles_.end());

    // --- ダメージ数値 --------------------------------------------------------
    for (DamageNumber& number : damageNumbers_) {
        number.life += dt;
        number.pos += number.velocity * dt;
        number.velocity.y += 900.0f * dt;
        number.velocity.x *= 0.94f;
    }
    damageNumbers_.erase(std::remove_if(damageNumbers_.begin(), damageNumbers_.end(),
                                        [](const DamageNumber& n) { return n.life >= n.maxLife; }),
                         damageNumbers_.end());

    // --- パーティクル --------------------------------------------------------
    for (Particle& particle : particles_) {
        particle.life += dt;
        particle.velocity.y += particle.gravity * dt;
        particle.pos += particle.velocity * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& p) { return p.life >= p.maxLife; }),
                     particles_.end());

    // --- 斬撃 / 波紋 / ポップアップ --------------------------------------------
    for (SlashEffect& slash : slashes_) slash.life += dt;
    slashes_.erase(std::remove_if(slashes_.begin(), slashes_.end(),
                                  [](const SlashEffect& s) { return s.life >= s.maxLife; }),
                   slashes_.end());

    for (RingEffect& ring : rings_) {
        ring.life += dt;
        const float t = math::Clamp(ring.life / ring.maxLife, 0.0f, 1.0f);
        ring.radius = ring.maxRadius * math::SmoothStep(t);
    }
    rings_.erase(std::remove_if(rings_.begin(), rings_.end(),
                                [](const RingEffect& r) { return r.life >= r.maxLife; }),
                 rings_.end());

    for (BoltEffect& bolt : bolts_) bolt.life += dt;
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(),
                                [](const BoltEffect& b) { return b.life >= b.maxLife; }),
                 bolts_.end());

    for (PopupText& popup : popups_) {
        popup.life += dt;
        popup.pos.y -= 46.0f * dt;
    }
    popups_.erase(std::remove_if(popups_.begin(), popups_.end(),
                                 [](const PopupText& p) { return p.life >= p.maxLife; }),
                  popups_.end());
}

void CombatSystem::DrawBehindActors(const Camera& camera) const
{
    // 衝撃波リング
    for (const RingEffect& ring : rings_) {
        const float t = math::Clamp(ring.life / ring.maxLife, 0.0f, 1.0f);
        const int alpha = static_cast<int>((1.0f - t) * 220.0f);
        const Vec2 screen = camera.WorldToScreen(ring.pos);
        SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(alpha, 0, 255));
        DrawOvalAA(screen.x, screen.y, ring.radius, ring.radius * 0.32f, 32,
                   draw::ToDx(ring.color), FALSE, ring.thickness * (1.0f - t * 0.6f));
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void CombatSystem::DrawFrontOfActors(const Camera& camera, bool showDamageNumbers) const
{
    // --- 飛び道具 -----------------------------------------------------------
    for (const Projectile& projectile : projectiles_) {
        const Vec2 screen = camera.WorldToScreen(projectile.pos);
        switch (projectile.kind) {
        case 1: { // 衝撃波
            draw::Glow(screen.x, screen.y, projectile.radius * 1.4f, projectile.color, 170, 4);
            const Rect body = Rect::FromCenter(screen.x, screen.y, projectile.radius * 0.9f,
                                               projectile.radius * 2.2f);
            draw::FillRect(body, projectile.color, 200);
            break;
        }
        case 2: { // 刃
            const float angle = std::atan2(projectile.velocity.y, projectile.velocity.x);
            const Vec2 tip(screen.x + std::cos(angle) * projectile.radius * 1.8f,
                           screen.y + std::sin(angle) * projectile.radius * 1.8f);
            const Vec2 a(screen.x - std::sin(angle) * projectile.radius,
                         screen.y + std::cos(angle) * projectile.radius);
            const Vec2 b(screen.x + std::sin(angle) * projectile.radius,
                         screen.y - std::cos(angle) * projectile.radius);
            draw::Glow(screen.x, screen.y, projectile.radius * 1.2f, projectile.color, 150, 3);
            draw::Triangle(a, b, tip, projectile.color, true, 230);
            break;
        }
        case 3: { // 金剛杵（両端に三つ叉の爪がある短い杵）
            const float angle = std::atan2(projectile.velocity.y, projectile.velocity.x);
            const Vec2 u(std::cos(angle), std::sin(angle));
            const Vec2 n(-u.y, u.x);
            const float len = projectile.radius * 1.6f;
            draw::Glow(screen.x, screen.y, projectile.radius * 1.5f, projectile.color, 170, 4);
            const Vec2 a(screen.x - u.x * len, screen.y - u.y * len);
            const Vec2 b(screen.x + u.x * len, screen.y + u.y * len);
            draw::Line(a.x, a.y, b.x, b.y, ColorRGB(60, 40, 16), projectile.radius * 0.55f, 255);
            draw::Line(a.x, a.y, b.x, b.y, projectile.color, projectile.radius * 0.32f, 255);
            draw::Circle(screen.x, screen.y, projectile.radius * 0.42f, palette::kWhite, true, 1.0f, 230);
            for (int end = -1; end <= 1; end += 2) {
                const Vec2 base(screen.x + u.x * len * 0.62f * static_cast<float>(end),
                                screen.y + u.y * len * 0.62f * static_cast<float>(end));
                for (int prong = -1; prong <= 1; ++prong) {
                    const float spread = projectile.radius * 0.5f * static_cast<float>(prong);
                    const Vec2 tip(base.x + u.x * projectile.radius * 0.9f * static_cast<float>(end) + n.x * spread,
                                   base.y + u.y * projectile.radius * 0.9f * static_cast<float>(end) + n.y * spread);
                    draw::Line(base.x, base.y, tip.x, tip.y, projectile.color, 3.0f, 255);
                }
            }
            break;
        }
        case 4: { // 光輪（地面すれすれを走る輪）
            const float r = projectile.radius;
            draw::Glow(screen.x, screen.y, r * 1.5f, projectile.color, 160, 4);
            DrawOvalAA(screen.x, screen.y, r, r * 0.9f, 32, draw::ToDx(projectile.color), FALSE, r * 0.22f);
            DrawOvalAA(screen.x, screen.y, r * 0.72f, r * 0.64f, 32, draw::ToDx(palette::kWhite), FALSE, 2.0f);
            const float spin = projectile.life * 9.0f;
            for (int i = 0; i < 6; ++i) {
                const float a = spin + static_cast<float>(i) * 1.0471976f;
                const float c = std::cos(a), s = std::sin(a);
                draw::Triangle(Vec2(screen.x + c * r * 1.02f, screen.y + s * r * 0.92f),
                               Vec2(screen.x + std::cos(a + 0.3f) * r * 1.02f, screen.y + std::sin(a + 0.3f) * r * 0.92f),
                               Vec2(screen.x + std::cos(a + 0.15f) * r * 1.4f, screen.y + std::sin(a + 0.15f) * r * 1.26f),
                               projectile.color, true, 230);
            }
            break;
        }
        default: { // 球
            draw::Glow(screen.x, screen.y, projectile.radius * 1.6f, projectile.color, 190, 5);
            draw::Circle(screen.x, screen.y, projectile.radius * 0.6f, palette::kWhite, true, 1.0f, 230);
            break;
        }
        }
    }

    // --- 雷 -----------------------------------------------------------------
    for (const BoltEffect& bolt : bolts_) {
        const float t = math::Clamp(bolt.life / bolt.maxLife, 0.0f, 1.0f);
        const int alpha = static_cast<int>((1.0f - t * t) * 255.0f);
        const Vec2 top = camera.WorldToScreen(Vec2(bolt.x, bolt.topY));
        const Vec2 bottom = camera.WorldToScreen(Vec2(bolt.x, bolt.bottomY));
        // 稲妻の折れ目（数フレームごとに形を変えてちらつかせる）
        const int flicker = bolt.seed + static_cast<int>(bolt.life * 30.0f);
        constexpr int kSegments = 11;
        Vec2 points[kSegments + 1];
        for (int i = 0; i <= kSegments; ++i) {
            const float k = static_cast<float>(i) / static_cast<float>(kSegments);
            const unsigned int h = static_cast<unsigned int>(flicker * 2654435761u + static_cast<unsigned int>(i) * 40503u);
            const float jitter = (i == 0 || i == kSegments) ? 0.0f
                               : (static_cast<float>(h % 1000u) / 1000.0f - 0.5f) * 58.0f;
            points[i] = Vec2(top.x + jitter, math::Lerp(top.y, bottom.y, k));
        }
        SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(alpha / 3, 0, 255));
        DrawBoxAA(bottom.x - 46.0f, top.y, bottom.x + 46.0f, bottom.y, draw::ToDx(bolt.color), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(alpha, 0, 255));
        for (int i = 0; i < kSegments; ++i) {
            DrawLineAA(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y, draw::ToDx(bolt.color), 14.0f);
        }
        for (int i = 0; i < kSegments; ++i) {
            DrawLineAA(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y, draw::ToDx(palette::kWhite), 5.0f);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        draw::Glow(bottom.x, bottom.y, 90.0f, bolt.color, static_cast<int>(200.0f * (1.0f - t)), 4);
    }

    // --- 斬撃エフェクト ------------------------------------------------------
    for (const SlashEffect& slash : slashes_) {
        const float t = math::Clamp(slash.life / slash.maxLife, 0.0f, 1.0f);
        const int alpha = static_cast<int>((1.0f - t) * 235.0f);
        const Vec2 screen = camera.WorldToScreen(slash.pos);
        const float dir = static_cast<float>(slash.facing >= 0 ? 1 : -1);
        const float radius = slash.radius * (0.7f + t * 0.5f);

        SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(alpha, 0, 255));
        const unsigned int color = draw::ToDx(slash.color);
        const unsigned int white = draw::ToDx(palette::kWhite);

        switch (slash.style) {
        case 1: { // 横薙ぎ
            const float h = radius * 0.30f;
            DrawTriangleAA(screen.x - radius * dir * 0.2f, screen.y - h,
                           screen.x + radius * dir, screen.y - h * 0.2f,
                           screen.x + radius * dir, screen.y + h * 0.2f, color, TRUE);
            DrawTriangleAA(screen.x - radius * dir * 0.2f, screen.y - h,
                           screen.x - radius * dir * 0.2f, screen.y + h,
                           screen.x + radius * dir, screen.y + h * 0.2f, color, TRUE);
            break;
        }
        case 2: { // 突き
            const float h = radius * 0.16f;
            DrawTriangleAA(screen.x, screen.y - h, screen.x, screen.y + h,
                           screen.x + radius * dir * 1.3f, screen.y, color, TRUE);
            DrawLineAA(screen.x, screen.y, screen.x + radius * dir * 1.3f, screen.y, white, 3.0f);
            break;
        }
        case 3: { // 連撃（小さな十字）
            const float h = radius * 0.5f;
            DrawLineAA(screen.x - h * 0.6f, screen.y - h, screen.x + h * 0.6f, screen.y + h, color, 6.0f);
            DrawLineAA(screen.x - h * 0.6f, screen.y + h, screen.x + h * 0.6f, screen.y - h, color, 6.0f);
            break;
        }
        case 4: { // 回転
            DrawCircleAA(screen.x, screen.y, radius * 0.9f, 32, color, FALSE, 10.0f * (1.0f - t));
            break;
        }
        default: { // 縦斬り
            const float w = radius * 0.28f;
            DrawTriangleAA(screen.x - w, screen.y - radius * 0.9f,
                           screen.x + w * dir, screen.y - radius * 0.2f,
                           screen.x + w * dir * 1.4f, screen.y + radius * 0.9f, color, TRUE);
            DrawLineAA(screen.x - w * 0.4f, screen.y - radius * 0.9f,
                       screen.x + w * dir * 1.1f, screen.y + radius * 0.9f, white, 4.0f);
            break;
        }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // --- パーティクル --------------------------------------------------------
    for (const Particle& particle : particles_) {
        const float t = math::Clamp(particle.life / particle.maxLife, 0.0f, 1.0f);
        const int alpha = static_cast<int>((1.0f - t) * 255.0f);
        const Vec2 screen = camera.WorldToScreen(particle.pos);
        const float size = particle.size * (1.0f - t * 0.5f);

        SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(alpha, 0, 255));
        if (particle.kind == 1) {
            DrawCircleAA(screen.x, screen.y, size, 12, draw::ToDx(particle.color), TRUE);
        } else if (particle.kind == 2) {
            DrawLineAA(screen.x, screen.y, screen.x - particle.velocity.x * 0.02f,
                       screen.y - particle.velocity.y * 0.02f, draw::ToDx(particle.color), size * 0.5f);
        } else {
            DrawBoxAA(screen.x - size, screen.y - size, screen.x + size, screen.y + size,
                      draw::ToDx(particle.color), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // --- ダメージ数値 --------------------------------------------------------
    if (showDamageNumbers) {
        for (const DamageNumber& number : damageNumbers_) {
            const float t = math::Clamp(number.life / number.maxLife, 0.0f, 1.0f);
            const int alpha = static_cast<int>((1.0f - t * t) * 255.0f);
            const Vec2 screen = camera.WorldToScreen(number.pos);

            const bool heal = number.value < 0;
            const int value = heal ? -number.value : number.value;
            const std::string text = heal ? str::Format("+%d", value) : str::Format("%d", value);
            const FontSize size = number.critical ? FontSize::Huge : FontSize::Large;

            if (number.critical) {
                draw::TextAlpha(FontSize::Small, screen.x, screen.y - 34.0f, palette::kCritical,
                                "CRITICAL", alpha, draw::TextAlign::Center);
            }
            draw::TextAlpha(size, screen.x + 3.0f, screen.y + 3.0f, palette::kBlack, text,
                            math::ClampInt(alpha / 2, 0, 255), draw::TextAlign::Center);
            draw::TextAlpha(size, screen.x, screen.y, number.color, text, alpha, draw::TextAlign::Center);
        }
    }

    // --- ポップアップ --------------------------------------------------------
    for (const PopupText& popup : popups_) {
        const float t = math::Clamp(popup.life / popup.maxLife, 0.0f, 1.0f);
        const int alpha = static_cast<int>((1.0f - t * t) * 255.0f);
        const Vec2 screen = camera.WorldToScreen(popup.pos);
        draw::TextAlpha(popup.large ? FontSize::Large : FontSize::Medium, screen.x, screen.y,
                        popup.color, popup.text, alpha, draw::TextAlign::Center);
    }
}

void CombatSystem::DrawDebugHitBoxes(const Camera& camera) const
{
    for (const HitBox& hitBox : hitBoxes_) {
        const Rect screen = camera.WorldToScreen(hitBox.area);
        const ColorRGB color = (hitBox.team == Team::Player) ? ColorRGB(90, 255, 160) : ColorRGB(255, 90, 90);
        draw::StrokeRect(screen, color, 2.0f, 180);
    }
}

} // namespace ecl
