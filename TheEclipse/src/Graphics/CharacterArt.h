//==============================================================================
// CharacterArt.h : キャラクター描画
//   スプライトがあればそれを描画し、無ければ図形ベースの代替表示を行う。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Game/WeaponType.h"
#include "Graphics/Animation.h"

namespace ecl {

//------------------------------------------------------------------------------
// 見た目の種別
//------------------------------------------------------------------------------
enum class ArtStyle
{
    Humanoid,  // プレイヤー / 人型
    Knight,    // 鎧の騎士（マント付き）
    Beast,     // 四足獣
    Golem,     // 岩石系
    Imp,       // 小型
    Wisp       // 浮遊体
};

//------------------------------------------------------------------------------
// 描画パラメータ
//------------------------------------------------------------------------------
struct ActorArt
{
    ArtStyle style = ArtStyle::Humanoid;
    ColorRGB main = ColorRGB(70, 90, 130);
    ColorRGB accent = ColorRGB(220, 230, 245);
    ColorRGB trim = ColorRGB(64, 206, 255);
    WeaponType weapon = WeaponType::OneHandSword;
    bool hasWeapon = true;
    bool hasShield = false;
    // 二刀流で左手にも武器を持つ
    bool hasOffHandWeapon = false;
    float scale = 1.0f;
};

// 画面座標の矩形に収めてキャラクターを描画する
//   animator が有効なスプライトを持つ場合はそちらを優先
void DrawActor(const Rect& screenRect, int facing, PoseKind pose, float phase,
               const ActorArt& art, const Animator* animator = nullptr,
               int alpha = 255, float flash = 0.0f);

// 武器単体の描画（手の位置と角度を指定）
void DrawWeapon(const Vec2& handPos, float angleRad, int facing, float length,
                WeaponType type, const ColorRGB& metal, const ColorRGB& accent, int alpha = 255);

// 足元の影
void DrawActorShadow(float centerX, float groundY, float width, int alpha = 90);

} // namespace ecl
