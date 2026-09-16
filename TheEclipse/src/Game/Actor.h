//==============================================================================
// Actor.h : 戦闘に参加するキャラクターの基底
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Core/Camera.h"
#include "Game/Combat.h"
#include "Game/Stage.h"
#include "Game/Stats.h"
#include "Graphics/Animation.h"
#include "Graphics/CharacterArt.h"

namespace ecl {

class Actor
{
public:
    virtual ~Actor() = default;

    // --- 基本情報 -----------------------------------------------------------
    int   id = 0;
    Team  team = Team::Enemy;
    Vec2  pos;              // 足元中心
    Vec2  velocity;
    float halfWidth = 32.0f;
    float height = 150.0f;
    int   facing = 1;
    bool  onGround = false;
    bool  alive = true;

    float hp = 100.0f;
    float maxHp = 100.0f;
    Stats stats;

    // --- 表示 ---------------------------------------------------------------
    ActorArt art;
    Animator animator;
    PoseKind pose = PoseKind::Idle;

    // --- 状態タイマ ---------------------------------------------------------
    float hurtTimer = 0.0f;
    float invincibleTimer = 0.0f;
    float flashTimer = 0.0f;
    float deathTimer = 0.0f;

    Rect Bounds() const;
    Vec2 Center() const;
    float HpRatio() const;
    bool IsInvincible() const { return invincibleTimer > 0.0f; }

    // 被弾処理（与えたダメージを返す / 0 なら無効）
    virtual int ApplyHit(const HitBox& hitBox, CombatSystem& combat);
    // 直接ダメージ（トゲ等）
    virtual int ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat);
    virtual void OnDeath(CombatSystem& combat);

    void ApplyPhysics(float dt, const Stage& stage);
    void UpdateTimers(float dt);
    void FaceTowards(float targetX);

    // 足元の影＋本体を描画
    void DrawBody(const Camera& camera, int alpha = 255) const;

protected:
    // 死亡演出の進行度（0〜1）
    float DeathPhase() const;
};

// 次に使うアクター ID を発行
int IssueActorId();

} // namespace ecl
