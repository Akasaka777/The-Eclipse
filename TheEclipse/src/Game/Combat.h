//==============================================================================
// Combat.h : 攻撃判定・飛び道具・エフェクト
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Core/Camera.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 所属陣営
//------------------------------------------------------------------------------
enum class Team
{
    Player,
    Enemy
};

//------------------------------------------------------------------------------
// 攻撃判定
//   攻撃者のステータスを複製して持つため、発生後に攻撃者が消えても解決できる。
//------------------------------------------------------------------------------
struct HitBox
{
    Rect     area;
    Team     team = Team::Player;
    int      sourceId = -1;
    float    attack = 0.0f;
    float    damageMultiplier = 1.0f;
    float    critRate = 0.0f;
    float    critDamage = 0.0f;
    float    knockback = 200.0f;
    float    hitStop = 0.05f;
    float    life = 0.08f;       // 判定の持続時間
    bool     launch = false;     // 打ち上げる
    bool     multiHit = false;   // 同一対象に複数回当たる
    float    hitInterval = 0.25f;
    ColorRGB color = ColorRGB(255, 255, 255);

    // 内部管理用
    float timer = 0.0f;
    std::vector<int> hitTargets;

    bool AlreadyHit(int targetId) const;
    void MarkHit(int targetId);
};

//------------------------------------------------------------------------------
// 飛び道具
//------------------------------------------------------------------------------
struct Projectile
{
    Vec2     pos;
    Vec2     velocity;
    float    radius = 16.0f;
    Team     team = Team::Enemy;
    int      sourceId = -1;
    float    attack = 0.0f;
    float    damageMultiplier = 1.0f;
    float    critRate = 0.0f;
    float    critDamage = 0.0f;
    float    knockback = 200.0f;
    float    life = 3.0f;
    bool     useGravity = false;
    bool     active = true;
    int      kind = 0; // 0:球 1:衝撃波 2:刃
    ColorRGB color = ColorRGB(255, 140, 90);
};

//------------------------------------------------------------------------------
// 表示用エフェクト
//------------------------------------------------------------------------------
struct DamageNumber
{
    Vec2     pos;
    Vec2     velocity;
    int      value = 0;
    float    life = 0.0f;
    float    maxLife = 0.9f;
    bool     critical = false;
    ColorRGB color = ColorRGB(255, 255, 255);
};

struct Particle
{
    Vec2     pos;
    Vec2     velocity;
    float    life = 0.0f;
    float    maxLife = 0.5f;
    float    size = 4.0f;
    float    gravity = 0.0f;
    ColorRGB color = ColorRGB(255, 255, 255);
    int      kind = 0; // 0:四角 1:円 2:線
};

struct SlashEffect
{
    Vec2     pos;
    float    angle = 0.0f;
    float    radius = 60.0f;
    float    life = 0.0f;
    float    maxLife = 0.22f;
    int      facing = 1;
    int      style = 0; // 0:縦 1:横 2:突き 3:連撃 4:回転
    ColorRGB color = ColorRGB(180, 230, 255);
};

struct RingEffect
{
    Vec2     pos;
    float    radius = 0.0f;
    float    maxRadius = 200.0f;
    float    life = 0.0f;
    float    maxLife = 0.4f;
    float    thickness = 6.0f;
    ColorRGB color = ColorRGB(255, 200, 120);
};

struct PopupText
{
    Vec2        pos;
    std::string text;
    float       life = 0.0f;
    float       maxLife = 1.4f;
    ColorRGB    color = ColorRGB(255, 255, 255);
    bool        large = false;
};

//------------------------------------------------------------------------------
// 戦闘系オブジェクトの管理
//------------------------------------------------------------------------------
class CombatSystem
{
public:
    void Clear();
    void Update(float dt);

    // --- 生成 ---------------------------------------------------------------
    void AddHitBox(const HitBox& hitBox);
    void AddProjectile(const Projectile& projectile);
    void AddDamageNumber(const Vec2& pos, int value, bool critical, const ColorRGB& color);
    void AddHealNumber(const Vec2& pos, int value);
    void AddSlash(const Vec2& pos, int facing, float radius, const ColorRGB& color, int style);
    void AddImpact(const Vec2& pos, const ColorRGB& color, int count = 12, float speed = 420.0f);
    void AddDust(const Vec2& pos, int facing, int count = 6);
    void AddRing(const Vec2& pos, float maxRadius, const ColorRGB& color, float duration = 0.4f);
    void AddPopup(const Vec2& pos, const std::string& text, const ColorRGB& color, bool large = false);

    // --- アクセス -----------------------------------------------------------
    std::vector<HitBox>&     HitBoxes() { return hitBoxes_; }
    std::vector<Projectile>& Projectiles() { return projectiles_; }

    // --- 描画 ---------------------------------------------------------------
    void DrawBehindActors(const Camera& camera) const;
    void DrawFrontOfActors(const Camera& camera, bool showDamageNumbers) const;
    // デバッグ用の判定枠表示
    void DrawDebugHitBoxes(const Camera& camera) const;

private:
    std::vector<HitBox>       hitBoxes_;
    std::vector<Projectile>   projectiles_;
    std::vector<DamageNumber> damageNumbers_;
    std::vector<Particle>     particles_;
    std::vector<SlashEffect>  slashes_;
    std::vector<RingEffect>   rings_;
    std::vector<PopupText>    popups_;
};

} // namespace ecl
