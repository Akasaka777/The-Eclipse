//==============================================================================
// SwordSkill.h : ソードスキル定義
//   スキル発動中はモーションが固定され、定義された時刻に判定が発生する。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Game/WeaponType.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// スキル中に発生する1撃分の判定
//------------------------------------------------------------------------------
struct SkillStrike
{
    float time = 0.0f;         // 発動からの秒数
    float damageMultiplier = 1.0f;
    float reach = 1.0f;        // 前方への判定距離（キャラ幅基準の倍率）
    float height = 1.0f;       // 判定の高さ（キャラ身長基準の倍率）
    float offsetY = 0.0f;      // judgement 中心の縦オフセット（身長比）
    float knockback = 180.0f;
    float forwardImpulse = 0.0f; // 自身の前進速度
    float hitStop = 0.05f;
    bool  launch = false;      // 打ち上げ
};

//------------------------------------------------------------------------------
// スキル本体
//------------------------------------------------------------------------------
struct SwordSkill
{
    int         id = 0;
    std::string name;
    std::string description;
    WeaponType  weapon = WeaponType::OneHandSword;
    float       mpCost = 10.0f;
    float       cooldown = 6.0f;
    float       duration = 0.6f;     // モーション全体の長さ
    float       invincibleUntil = 0.0f; // この時刻まで無敵
    float       moveLock = 1.0f;     // 1.0 で完全に移動不可
    ColorRGB    effectColor = ColorRGB(120, 220, 255);
    int         effectStyle = 0;     // 0:縦 1:横 2:突き 3:連撃 4:回転
    std::vector<SkillStrike> strikes;

    // --- スキルツリー上の位置 -------------------------------------------------
    int  column = 0;          // 0 = 攻撃系統 / 1 = 機動・範囲系統
    int  tier = 1;            // 1〜3（上から下へ）
    int  unlockCost = 1;      // 解放に必要なスキルポイント（0 は初期解放）
    int  requiredSkillId = 0; // 前提スキル（0 なら無し）

    float TotalMultiplier() const;
    bool  IsStarter() const { return unlockCost <= 0; }
};

// ツリーの段数と系統数
constexpr int kSkillTreeColumns = 2;
constexpr int kSkillTreeTiers = 3;
// 装備できるスキル数
constexpr int kSkillSlotCount = 4;

//------------------------------------------------------------------------------
// スキルのマスターデータ
//------------------------------------------------------------------------------
class SkillDatabase
{
public:
    static const SkillDatabase& Instance();

    const SwordSkill* Find(int id) const;
    // 武器種に対応するスキル一覧
    std::vector<const SwordSkill*> ForWeapon(WeaponType weapon) const;
    // 武器種の既定ロードアウト（4つ）
    std::vector<int> DefaultLoadout(WeaponType weapon) const;

    // --- スキルツリー ---------------------------------------------------------
    // 系統・段の順に並んだツリー（column * kSkillTreeTiers + tier - 1 の順）
    std::vector<const SwordSkill*> TreeForWeapon(WeaponType weapon) const;
    // 指定位置のノード（無ければ nullptr）
    const SwordSkill* NodeAt(WeaponType weapon, int column, int tier) const;
    // 初期解放スキル（各武器の起点）
    std::vector<int> StarterSkillIds() const;
    // 全スキル
    const std::vector<SwordSkill>& AllSkills() const { return skills_; }

private:
    SkillDatabase();

    std::vector<SwordSkill> skills_;
};

} // namespace ecl
