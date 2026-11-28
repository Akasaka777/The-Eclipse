#include "Game/Pickup.h"

#include "Common/MathUtil.h"
#include "Core/GameConfig.h"
#include "Core/DxInclude.h"
#include "Game/Consumable.h"
#include "Game/Stage.h"
#include "Graphics/DrawUtil.h"

#include <algorithm>
#include <cmath>

namespace ecl {

namespace {

constexpr float kGravity = 1800.0f;
constexpr float kMagnetSpeed = 900.0f;
constexpr float kGatherSpeed = 1500.0f;

bool Attracts(PickupKind kind)
{
    return kind == PickupKind::Col || kind == PickupKind::Item || kind == PickupKind::Crystal;
}

} // namespace

void PickupField::Add(PickupKind kind, const Vec2& at, float z, int amount, int itemId, const EquipmentItem& gear)
{
    Pickup p;
    p.kind = kind;
    p.pos = at;
    p.z = z;
    p.amount = amount;
    p.itemId = itemId;
    p.gear = gear;
    // 倒した場所から少し散らばって飛び出す
    p.velocity = Vec2(math::RandFloat(-170.0f, 170.0f), 0.0f);
    p.height = math::RandFloat(40.0f, 90.0f);
    p.heightSpeed = math::RandFloat(380.0f, 560.0f);
    if (kind == PickupKind::Skill || kind == PickupKind::Gear) {
        // スキルと装備はあまり散らばらない（見つけやすく）
        p.velocity.x *= 0.4f;
    }
    pickups_.push_back(p);
}

int PickupField::Count(PickupKind kind) const
{
    int count = 0;
    for (const Pickup& p : pickups_) {
        if (!p.collected && p.kind == kind) ++count;
    }
    return count;
}

void PickupField::Update(float dt, const Stage& stage, const Vec2& playerPos, float playerZ, bool gatherAll,
                         std::vector<Pickup>& collected)
{
    for (Pickup& p : pickups_) {
        if (p.collected) continue;
        p.age += dt;

        // 跳ねながら地面に落ちる
        p.heightSpeed -= kGravity * dt;
        p.height += p.heightSpeed * dt;
        if (p.height <= 0.0f) {
            p.height = 0.0f;
            p.heightSpeed = (p.heightSpeed < -160.0f) ? -p.heightSpeed * 0.35f : 0.0f;
        }
        p.pos.x = stage.ClampX(p.pos.x + p.velocity.x * dt, 24.0f);
        p.velocity.x *= std::exp(-3.0f * dt);

        const float dx = playerPos.x - p.pos.x;
        const float dz = playerZ - p.z;
        const float dist = std::sqrt(dx * dx + dz * dz);
        const bool ready = p.age >= kPickupDelay;

        // col・アイテム・強化結晶は吸い寄せられる（全滅後はフロアのどこからでも集まる）
        if (ready && Attracts(p.kind) && (gatherAll || dist < kPickupMagnetRange) && dist > 1.0f) {
            const float speed = gatherAll ? kGatherSpeed : kMagnetSpeed;
            const float step = math::MinF(dist, speed * dt);
            p.pos.x += dx / dist * step;
            p.z += dz / dist * step;
            p.velocity.x = 0.0f;
        }
        p.z = stage.ClampZ(p.z);
        p.pos.y = stage.GroundYAt(p.z);

        if (ready && math::Abs(playerPos.x - p.pos.x) < kPickupTouchRange
            && math::Abs(playerZ - p.z) < kPickupTouchRange) {
            p.collected = true;
            collected.push_back(p);
        }
    }
    pickups_.erase(std::remove_if(pickups_.begin(), pickups_.end(), [](const Pickup& p) { return p.collected; }),
                   pickups_.end());
}

void PickupField::Draw(const Camera& camera, float time) const
{
    for (const Pickup& p : pickups_) {
        const Vec2 ground = camera.WorldToScreen(p.pos);
        if (ground.x < -80.0f || ground.x > static_cast<float>(config::kScreenWidth) + 80.0f) continue;
        const float bob = (p.height <= 0.0f) ? std::sin(time * 4.0f + p.pos.x * 0.01f) * 4.0f : 0.0f;
        const Vec2 c(ground.x, ground.y - p.height - 16.0f - bob);

        // 足元の影
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
        DrawOvalAA(ground.x, ground.y, 16.0f, 5.0f, 16, draw::ToDx(ColorRGB(0, 0, 0)), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        switch (p.kind) {
        case PickupKind::Col: {
            const float r = (p.amount >= 100) ? 13.0f : 10.0f;
            draw::Circle(c.x, c.y, r + 2.0f, ColorRGB(90, 60, 10), true, 1.0f, 255);
            draw::Circle(c.x, c.y, r, ColorRGB(255, 206, 90), true, 1.0f, 255);
            draw::Circle(c.x, c.y, r * 0.62f, ColorRGB(226, 160, 50), false, 2.0f, 255);
            draw::Circle(c.x - r * 0.35f, c.y - r * 0.35f, r * 0.22f, palette::kWhite, true, 1.0f, 200);
            break;
        }
        case PickupKind::Item: {
            const ConsumableDef* def = ConsumableDatabase::Instance().Find(p.itemId);
            const ColorRGB color = def ? def->color : ColorRGB(120, 220, 140);
            draw::Glow(c.x, c.y, 22.0f, color, 70, 3);
            draw::Circle(c.x, c.y + 3.0f, 11.0f, ColorRGB(20, 20, 28), true, 1.0f, 255);
            draw::Circle(c.x, c.y + 3.0f, 9.0f, color, true, 1.0f, 255);
            draw::FillRect(Rect(c.x - 4.0f, c.y - 13.0f, c.x + 4.0f, c.y - 5.0f), ColorRGB(220, 210, 190), 255);
            break;
        }
        case PickupKind::Crystal: {
            const ColorRGB blue(120, 210, 255);
            draw::Glow(c.x, c.y, 24.0f, blue, 90, 3);
            draw::Triangle(Vec2(c.x, c.y - 14.0f), Vec2(c.x - 9.0f, c.y), Vec2(c.x + 9.0f, c.y), blue, true, 255);
            draw::Triangle(Vec2(c.x, c.y + 14.0f), Vec2(c.x - 9.0f, c.y), Vec2(c.x + 9.0f, c.y), blue.Scaled(0.7f), true, 255);
            draw::Line(c.x, c.y - 14.0f, c.x, c.y + 14.0f, palette::kWhite, 1.0f, 160);
            break;
        }
        case PickupKind::Skill: {
            // 回る輪をまとった光の玉
            const ColorRGB glow(150, 240, 255);
            const Vec2 o(c.x, c.y - 10.0f);
            draw::Glow(o.x, o.y, 46.0f, glow, 120, 5);
            draw::Circle(o.x, o.y, 15.0f, ColorRGB(40, 120, 170), true, 1.0f, 255);
            draw::Circle(o.x, o.y, 11.0f, glow, true, 1.0f, 255);
            draw::Circle(o.x - 4.0f, o.y - 4.0f, 4.0f, palette::kWhite, true, 1.0f, 230);
            for (int i = 0; i < 3; ++i) {
                const float a = time * 2.4f + static_cast<float>(i) * 2.094f;
                draw::Circle(o.x + std::cos(a) * 24.0f, o.y + std::sin(a) * 9.0f, 3.0f, glow, true, 1.0f, 220);
            }
            draw::Text(FontSize::Tiny, o.x, o.y - 44.0f, glow, "SKILL", draw::TextAlign::Center);
            break;
        }
        case PickupKind::Gear: {
            // 光の柱と、武器 / 防具の形
            const ColorRGB gold(255, 214, 120);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 70);
            DrawBoxAA(c.x - 10.0f, c.y - 150.0f, c.x + 10.0f, ground.y, draw::ToDx(gold), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            draw::Glow(c.x, c.y - 6.0f, 34.0f, gold, 110, 4);
            if (p.gear.IsWeapon()) {
                draw::Line(c.x - 14.0f, c.y + 8.0f, c.x + 14.0f, c.y - 22.0f, ColorRGB(230, 236, 250), 5.0f, 255);
                draw::Line(c.x - 10.0f, c.y - 2.0f, c.x - 2.0f, c.y + 6.0f, ColorRGB(150, 110, 60), 5.0f, 255);
            } else {
                draw::FillRect(Rect::FromCenter(c.x, c.y - 6.0f, 22.0f, 24.0f), ColorRGB(20, 20, 28), 255);
                draw::FillRect(Rect::FromCenter(c.x, c.y - 6.0f, 18.0f, 20.0f), gold.Scaled(0.85f), 255);
            }
            break;
        }
        }
    }
}

} // namespace ecl
