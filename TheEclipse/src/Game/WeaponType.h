//==============================================================================
// WeaponType.h : 武器種別（描画とゲームロジックの双方から参照する）
//==============================================================================
#pragma once

namespace ecl {

enum class WeaponType
{
    OneHandSword, // 片手剣
    OneHandMace,  // 片手棍
    Dagger,       // 短剣
    Rapier,       // 細剣
    Spear,        // 槍
    Count
};

// 日本語名
const char* WeaponTypeName(WeaponType type);
// 攻撃レンジ倍率（1.0 = 片手剣基準）
float WeaponReachScale(WeaponType type);
// 攻撃速度倍率
float WeaponSpeedScale(WeaponType type);

} // namespace ecl
