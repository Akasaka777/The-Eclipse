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
    Wisp,      // 浮遊体
    Dragon,    // 竜（翼・角・長い首と尾）
    Statue     // 金属の仏像（上半身だけ・六本の腕。ヴァジュラ・ラグナシュラ）
};

// 仏像の攻撃の構え（DrawActor の motionVariant に入れる）
constexpr int kStatueMotionNone = 0;
constexpr int kStatueMotionVolley = 1;   // 上の腕を前へ突き出す（金剛杵の雨）
constexpr int kStatueMotionThunder = 2;  // 上の腕を高く掲げる（天雷）
constexpr int kStatueMotionHalo = 3;     // 下の腕を大きく開く（光輪）
constexpr int kStatueMotionDive = 4;     // 四本の腕を下へ叩きつける（降魔の掌）

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
//   motionVariant : 通常攻撃の段数 / スキルの演出種別（武器モーションの選択に使う）
void DrawActor(const Rect& screenRect, int facing, PoseKind pose, float phase,
               const ActorArt& art, const Animator* animator = nullptr,
               int alpha = 255, float flash = 0.0f, int motionVariant = 0);

// 武器単体の描画（手の位置と角度を指定）
void DrawWeapon(const Vec2& handPos, float angleRad, int facing, float length,
                WeaponType type, const ColorRGB& metal, const ColorRGB& accent, int alpha = 255);

// 足元の影
void DrawActorShadow(float centerX, float groundY, float width, int alpha = 90);

} // namespace ecl
