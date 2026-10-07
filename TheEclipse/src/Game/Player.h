//==============================================================================
// Player.h : プレイヤーキャラクター
//   通常攻撃（3段コンボ）＋ ソードスキル（モーション固定）のハイブリッド。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/Consumable.h"
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

//------------------------------------------------------------------------------
// 戦闘中にアイテムを使った結果
//------------------------------------------------------------------------------
enum class ItemUseResult
{
    Used,       // 使った（呼び出し側で所持数を 1 減らす）
    Cooldown,   // 直前に使ったばかり
    NoEffect,   // HP / MP が満タンで回復の意味が無い
    Unusable    // 戦闘不能・素材など、使えない状態や種類
};

class Player : public Actor
{
public:
    Player();

    // 装備・レベルを反映して初期化
    void Setup(const PlayerData& data);
    // 戦闘中の装備変更・装備破損を反映する（HP / MP と位置は維持）
    void RefreshEquipment(const PlayerData& data);
    // フロア開始位置へ配置（HP/MP は維持）
    void PlaceAt(const Vec2& position);
    // HP/MP を全快
    void FullHeal();

    void Update(float dt, const Stage& stage, CombatSystem& combat,
                const Input& input, bool controlEnabled);
    // 奥行き方向へ移動中か（演出用）
    bool IsMovingDepth() const { return depthMoving_; }
    void Draw(const Camera& camera) const;

    int  ApplyHit(const HitBox& hitBox, CombatSystem& combat) override;
    void OnDeath(CombatSystem& combat) override;

    // --- 状態参照 -----------------------------------------------------------
    PlayerState State() const { return state_; }
    bool IsDead() const { return !alive; }
    bool IsGuarding() const { return guarding_; }
    // 神聖剣＋盾でガードが完全無効化になる状態か
    bool HasPerfectGuard() const { return perfectGuard_; }
    float Mp() const { return mp_; }
    float MaxMp() const { return maxMp_; }
    float MpRatio() const;

    // --- パリィ -------------------------------------------------------------
    // ガード中に攻撃ボタンを押すと受け流しの受付時間が発生する
    bool  TryParry(CombatSystem& combat);
    bool  CanParry() const;
    bool  IsParryActive() const { return parryWindow_ > 0.0f; }
    float ParryWindowRatio() const;
    float ParryCooldown() const { return parryCooldown_; }
    // パリィ成功をシーンへ 1 度だけ通知する（攻撃者の ID を受け取る）
    bool  ConsumeParrySignal(int* outSourceId = nullptr);

    const SwordSkill* Skill(int index) const;
    float SkillCooldown(int index) const;
    float SkillCooldownRatio(int index) const;
    bool  CanUseSkill(int index) const;
    // スキル発動（成功したら true）
    bool  UseSkill(int index, CombatSystem& combat);

    // 直前フレームで攻撃判定を出したか（コンボ表示のトリガ）
    bool JustAttacked() const { return justAttacked_; }
    WeaponType Weapon() const { return weapon_; }
    // 武器を装備しているか（素手ではソードスキルを使えない）
    bool HasWeapon() const { return hasWeapon_; }

    // --- 戦闘開始時バフ（旅人の護符）-----------------------------------------
    bool  HasOpeningBuff() const { return openingTimer_ > 0.0f; }
    float OpeningBuffRemain() const { return openingTimer_; }
    float OpeningBuffRate() const { return openingAttackRate_; }
    // バフを乗せた攻撃力（判定を出すときに使う）
    float AttackPower() const;
    // アイテムのバフを乗せた防御力・移動速度・クリティカル率
    float DefensePower() const;
    float MoveSpeed() const;
    float CritRate() const;

    // --- アイテム -----------------------------------------------------------
    // 回復・バフアイテムを使う。Used のときだけ効果が出る（所持数は呼び出し側で減らす）
    ItemUseResult UseItem(const ConsumableDef& item, CombatSystem& combat);
    float ItemCooldown() const { return itemCooldown_; }
    // 効いているバフアイテム（無ければ nullptr）
    const ConsumableDef* ItemBuff() const { return (itemBuffTimer_ > 0.0f) ? itemBuff_ : nullptr; }
    float ItemBuffRemain() const { return itemBuffTimer_; }
    float ItemBuffRatio() const;

private:
    // 装備由来の値を反映する（Setup / RefreshEquipment の共通処理）
    void ApplyEquipment(const PlayerData& data);
    void UpdateNormal(float dt, CombatSystem& combat, const Input& input, bool controlEnabled,
                      const Stage& stage);
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
    // 武器を持っているか（素手ではソードスキルを封じる）
    bool  hasWeapon_ = true;
    float attackSpeedFactor_ = 1.0f;

    // 戦闘開始時に一定時間だけ攻撃力が上がる（旅人の護符）
    float openingAttackRate_ = 0.0f;
    float openingDuration_ = 0.0f;
    float openingTimer_ = 0.0f;

    // バフアイテム（1 つだけ。別のバフを使うと上書き）
    const ConsumableDef* itemBuff_ = nullptr;
    float itemBuffTimer_ = 0.0f;
    float itemCooldown_ = 0.0f;

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
    bool  depthMoving_ = false;

    // パリィ
    float parryWindow_ = 0.0f;    // 受付の残り時間
    float parryCooldown_ = 0.0f;  // 再発動までの硬直
    float parryFlash_ = 0.0f;     // 成功演出の残り時間
    bool  parrySignal_ = false;   // シーンへ未通知の成功があるか
    bool  perfectGuard_ = false;  // ユニークスキル「神聖剣」＋盾で有効
    int   parrySourceId_ = -1;    // 受け流した攻撃の発生元
    float jumpBuffer_ = 0.0f;
    float coyoteTimer_ = 0.0f;
    float afterImageTimer_ = 0.0f;
};

} // namespace ecl
