//==============================================================================
// Enemy.h : 雑魚敵（索敵 → 接近 → 予備動作 → 攻撃 → 硬直 の状態機械）
//==============================================================================
#pragma once

#include "Game/Actor.h"
#include "Game/EnemyDatabase.h"

namespace ecl {

enum class EnemyState
{
    Idle,
    Chase,
    Windup,
    Attack,
    Recover,
    Hurt,
    Dead
};

class Enemy : public Actor
{
public:
    void Setup(const EnemyDef& def, const Vec2& position, float powerScale);

    void Update(float dt, const Stage& stage, CombatSystem& combat,
                const Vec2& playerPos, bool playerAlive);
    void Draw(const Camera& camera) const;

    int  ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat) override;
    void OnDeath(CombatSystem& combat) override;
    void Stagger(float duration) override;

    // 死亡演出が終わり、リストから除去してよいか
    bool IsRemovable() const;

    const EnemyDef* Def() const { return def_; }
    int ExpReward() const;
    int ColReward() const;

    // 撃破報酬を加算済みか（シーン側で 1 度だけ処理するために使う）
    bool rewardGranted = false;

private:
    void SpawnAttack(CombatSystem& combat);
    void UpdatePose();

    const EnemyDef* def_ = nullptr;
    EnemyState state_ = EnemyState::Idle;
    float stateTimer_ = 0.0f;
    float attackCooldown_ = 0.0f;
    float hoverPhase_ = 0.0f;
    float baseY_ = 0.0f;      // 初期の浮遊高度
    float hoverY_ = 0.0f;     // 現在の浮遊高度（戦闘中はプレイヤーへ寄る）
    float powerScale_ = 1.0f;
    float staggerDuration_ = 0.3f;  // のけぞりの長さ（パリィ時に延長される）
    float healthBarTimer_ = 0.0f;
};

} // namespace ecl
