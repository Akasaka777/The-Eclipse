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

    Stats& operator+=(const Stats& o);
    Stats  operator+(const Stats& o) const;
    Stats  Scaled(float factor) const;

    // 装備比較などで使う総合力
    int Power() const;
};

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
