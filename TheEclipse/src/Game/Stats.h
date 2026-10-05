//==============================================================================
// Stats.h : キャラクターおよび装備の能力値
//==============================================================================
#pragma once

#include "Game/WeaponType.h"

#include <string>

namespace ecl {

//------------------------------------------------------------------------------
// 能力値（キャラ基礎値と装備値を同じ構造で扱う）
//------------------------------------------------------------------------------
struct Stats
{
    float maxHp = 0.0f;
    float maxMp = 0.0f;
    float attack = 0.0f;
    float defense = 0.0f;
    float critRate = 0.0f;     // 0.0〜1.0
    float critDamage = 0.0f;   // 追加倍率（0.5 = +50%）
    float mpRegen = 0.0f;      // 毎秒回復量
    float moveSpeed = 0.0f;    // px/s の加算
    float attackSpeed = 0.0f;  // 倍率の加算（0.1 = +10%）

    // --- アクセサリーで付く能力値 ---------------------------------------------
    //   いずれも加算（0.05 = +5%）。魔法・属性・命中の仕組みは今後の実装予定で、
    //   現時点では画面に出るだけで戦闘結果には効かない。
    float magicAttack = 0.0f;   // 魔法攻撃力
    float magicDefense = 0.0f;  // 魔法防御力
    float accuracy = 0.0f;      // 命中率
    float fireResist = 0.0f;    // 火属性ダメージの軽減
    float waterResist = 0.0f;   // 水属性ダメージの軽減

    Stats& operator+=(const Stats& o);
    Stats  operator+(const Stats& o) const;
    Stats  Scaled(float factor) const;

    // 装備比較などで使う総合力
    //   魔法・属性・命中はまだ戦闘に効かないので総合力には含めない。
    int Power() const;
};

//------------------------------------------------------------------------------
// 倍率で効く補正（アクセサリーのバフ）
//   0.05 = +5%。足し合わせたうえで素のステータスに掛ける。
//------------------------------------------------------------------------------
struct StatRates
{
    float maxHp = 0.0f;
    float maxMp = 0.0f;
    float attack = 0.0f;
    float defense = 0.0f;
    float moveSpeed = 0.0f;   // 「素早さ」として扱う

    StatRates& operator+=(const StatRates& o);
    bool Empty() const;
};

// 倍率補正を適用した能力値を返す
Stats ApplyRates(const Stats& base, const StatRates& rates);

//------------------------------------------------------------------------------
// ダメージ計算
//------------------------------------------------------------------------------
struct DamageResult
{
    int  value = 0;
    bool critical = false;
    bool guarded = false;
};

// attack : 攻撃側攻撃力 / defense : 防御側防御力 / multiplier : スキル倍率
DamageResult CalculateDamage(float attack, float defense, float multiplier,
                             float critRate, float critDamage, bool canCrit = true);

// ステータス項目名（UI 表示用）
std::string StatsSummaryLine(const Stats& stats);

} // namespace ecl
