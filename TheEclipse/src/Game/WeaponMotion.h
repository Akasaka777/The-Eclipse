//==============================================================================
// WeaponMotion.h : 武器種ごとの攻撃モーション定義
//   ・通常攻撃のコンボ構成（段数 / 判定 / 硬直）
//   ・見た目の振り方（武器の角度・突き出し・踏み込み）
//   の両方をここにまとめ、ゲームロジックと描画の双方から参照する。
//==============================================================================
#pragma once

#include "Game/WeaponType.h"

namespace ecl {

//------------------------------------------------------------------------------
// 通常攻撃 1 段分の定義
//------------------------------------------------------------------------------
struct ComboStep
{
    const char* name;        // 段の呼び名
    float duration;          // 段全体の長さ（秒 / 攻撃速度で割って使う）
    float hitTime;           // 判定が出る時刻（秒）
    float damageMultiplier;  // 攻撃力倍率
    float reach;             // 前方への判定距離（キャラ幅基準）
    float height;            // 判定の高さ（身長基準）
    float offsetY;           // 判定中心の縦オフセット（身長基準 / 正で上）
    float depthScale;        // 奥行き方向の当たり幅の倍率
    float knockback;         // 吹き飛ばし
    float forwardImpulse;    // 自身の前進速度
    float hitStop;           // ヒットストップ
    int   effectStyle;       // 斬撃エフェクト 0:縦 1:横 2:突き 3:連撃 4:回転
};

//------------------------------------------------------------------------------
// 武器種ごとのコンボ全体
//------------------------------------------------------------------------------
struct ComboChain
{
    const ComboStep* steps = nullptr;
    int count = 0;
};

// 武器種のコンボを取得する
const ComboChain& WeaponCombo(WeaponType type);
// コンボの段数
int WeaponComboLength(WeaponType type);
// index 段目の定義（範囲外は丸めて返す）
const ComboStep& WeaponComboStep(WeaponType type, int index);

//------------------------------------------------------------------------------
// 描画用の姿勢
//   角度以外はすべて身長に対する比率で、facing = 1（右向き）を基準とする。
//------------------------------------------------------------------------------
struct WeaponSwing
{
    float angleDeg = 70.0f;   // 武器の角度（0 = 正面水平 / 90 = 真下 / 負 = 上）
    float reachOut = 0.0f;    // 手を前へ突き出す量
    float liftUp = 0.0f;      // 手を持ち上げる量
    float lean = 0.0f;        // 上体の前傾
    float crouch = 0.0f;      // 沈み込み
    float offHandDeg = 48.0f; // 二刀流のとき左手を主武器から何度ずらすか
};

// 通常攻撃のモーション（step = 段数 / t = 0〜1 の進行度）
WeaponSwing SampleComboSwing(WeaponType type, int step, float t);
// ソードスキルのモーション（style = SwordSkill::effectStyle）
WeaponSwing SampleSkillSwing(WeaponType type, int style, float t);

} // namespace ecl
