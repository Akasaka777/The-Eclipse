//==============================================================================
// Ability.h : プレイヤーの振り分けステータス（STR / AGI / VIT / INT / LUK）
//   レベルアップで貰えるステータスポイントを割り振って伸ばす。
//   1 ポイントあたりの効果は下の k***PerPoint をいじれば調整できる。
//==============================================================================
#pragma once

#include "Game/Stats.h"

#include <string>

namespace ecl {

enum class Ability
{
    Str,   // 攻撃力
    Agi,   // 移動速度と武器を振る速さ
    Vit,   // 最大 HP と防御力
    Int,   // クリティカル率とクリティカル倍率
    Luk,   // ドロップ率
    Count
};

constexpr int kAbilityCount = static_cast<int>(Ability::Count);

//------------------------------------------------------------------------------
// 1 ポイントあたりの効果（ここを変えればバランスを調整できる）
//------------------------------------------------------------------------------
constexpr float kStrAttackPerPoint     = 0.015f;  // 攻撃力 +1.5%
constexpr float kAgiMoveSpeedPerPoint  = 0.008f;  // 移動速度 +0.8%
constexpr float kAgiAttackSpeedPerPoint = 0.010f; // 攻撃速度 +1.0%（加算）
constexpr float kVitMaxHpPerPoint      = 0.012f;  // 最大 HP +1.2%
constexpr float kVitDefensePerPoint    = 0.015f;  // 防御力 +1.5%
constexpr float kIntCritRatePerPoint   = 0.004f;  // クリティカル率 +0.4%（加算）
constexpr float kIntCritDamagePerPoint = 0.010f;  // クリティカル倍率 +1.0%（加算）
constexpr float kLukDropRatePerPoint   = 0.020f;  // ドロップ率 +2.0%

// 初期値とレベルアップで貰えるポイント
constexpr int kAbilityInitialValue = 5;
constexpr int kAbilityPointsPerLevel = 3;
constexpr int kAbilityMaxValue = 999;

//------------------------------------------------------------------------------
// 5 つのステータス値
//------------------------------------------------------------------------------
struct AbilityScores
{
    int values[kAbilityCount] = { kAbilityInitialValue, kAbilityInitialValue,
                                  kAbilityInitialValue, kAbilityInitialValue,
                                  kAbilityInitialValue };

    int  Get(Ability ability) const;
    void Set(Ability ability, int value);
    void Add(Ability ability, int amount);
    int  Total() const;
};

// 表示名（"STR" など）と日本語の説明
const char* AbilityShortName(Ability ability);
const char* AbilityName(Ability ability);
// 一覧に並べる短い効果（詳細欄には AbilityEffectText を使う）
const char* AbilityShortEffect(Ability ability);
std::string AbilityEffectText(Ability ability);

// ステータスを反映した能力値を返す（base は 基礎 + 装備）
Stats ApplyAbilities(const Stats& base, const AbilityScores& scores);

// ドロップ率の倍率（LUK）
float AbilityDropRate(const AbilityScores& scores);

} // namespace ecl
