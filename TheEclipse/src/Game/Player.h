//==============================================================================
// Player.h : プレイヤーキャラクター
//   通常攻撃（3段コンボ）＋ ソードスキル（モーション固定）のハイブリッド。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/Actor.h"
#include "Game/PlayerData.h"
#include "Game/SwordSkill.h"

namespace ecl {

//------------------------------------------------------------------------------
// 行動状態
//------------------------------------------------------------------------------
enum class PlayerState
{
    Normal,
    Attack,
    Skill,
    Dash,
    Hurt,
    Dead
};

class Player : public Actor
{
public:
    Player();

    // 装備・レベルを反映して初期化
    void Setup(const PlayerData& data);
    // フロア開始位置へ配置（HP/MP は維持）
    void PlaceAt(const Vec2& position);
    // HP/MP を全快
    void FullHeal();

    void Update(float dt, const Stage& stage, CombatSystem& combat,
                const Input& input, bool controlEnabled);
    void Draw(const Camera& camera) const;

    int  ApplyHit(const HitBox& hitBox, CombatSystem& combat) override;
    void OnDeath(CombatSystem& combat) override;

    // --- 状態参照 -----------------------------------------------------------
    PlayerState State() const { return state_; }
    bool IsDead() const { return !alive; }
    bool IsGuarding() const { return guarding_; }
    float Mp() const { return mp_; }
    float MaxMp() const { return maxMp_; }
    float MpRatio() const;

    const SwordSkill* Skill(int index) const;
    float SkillCooldown(int index) const;
    float SkillCooldownRatio(int index) const;
    bool  CanUseSkill(int index) const;
    // スキル発動（成功したら true）
    bool  UseSkill(int index, CombatSystem& combat);

    // 直前フレームで攻撃判定を出したか（コンボ表示のトリガ）
    bool JustAttacked() const { return justAttacked_; }
    WeaponType Weapon() const { return weapon_; }

private:
    void UpdateNormal(float dt, CombatSystem& combat, const Input& input, bool controlEnabled);
    void UpdateAttack(float dt, CombatSystem& combat, const Input& input);
    void UpdateSkill(float dt, CombatSystem& combat);
    void UpdateDash(float dt);

    void StartAttack(CombatSystem& combat);
    void SpawnComboHitBox(CombatSystem& combat);
    void SpawnSkillStrike(const SkillStrike& strike, CombatSystem& combat);
    void UpdatePose();

    PlayerState state_ = PlayerState::Normal;

    float mp_ = 100.0f;
    float maxMp_ = 100.0f;

    WeaponType weapon_ = WeaponType::OneHandSword;
    float attackSpeedFactor_ = 1.0f;

    // 通常攻撃
    int   comboIndex_ = 0;
    float attackTimer_ = 0.0f;
    float attackDuration_ = 0.3f;
    bool  attackHitSpawned_ = false;
    bool  attackBuffered_ = false;
    float comboResetTimer_ = 0.0f;

    // ソードスキル
    const SwordSkill* currentSkill_ = nullptr;
    float skillTimer_ = 0.0f;
    size_t nextStrikeIndex_ = 0;
    float cooldowns_[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const SwordSkill* skills_[4] = { nullptr, nullptr, nullptr, nullptr };

    // 回避
    float dashTimer_ = 0.0f;
    float dashCooldown_ = 0.0f;

    bool  guarding_ = false;
    bool  justAttacked_ = false;
    float jumpBuffer_ = 0.0f;
    float coyoteTimer_ = 0.0f;
    float afterImageTimer_ = 0.0f;
};

} // namespace ecl
