//==============================================================================
// GameConfig.h : ゲーム全体の定数と実行時設定
//==============================================================================
#pragma once

#include "Common/Types.h"

namespace ecl {

//------------------------------------------------------------------------------
// 画面・物理定数
//------------------------------------------------------------------------------
namespace config {

constexpr int   kScreenWidth = 1920;
constexpr int   kScreenHeight = 1080;
constexpr int   kColorDepth = 32;
constexpr float kTargetFps = 60.0f;
// 1フレームの最大経過時間（処理落ち時のすり抜け防止）
constexpr float kMaxDeltaTime = 1.0f / 20.0f;

constexpr float kGravity = 2800.0f;      // px/s^2
constexpr float kMaxFallSpeed = 1800.0f; // px/s
constexpr float kJumpVelocity = -1150.0f;

constexpr const char* kWindowTitle = "The Eclipse";
constexpr const char* kAssetRoot = "assets";

} // namespace config

//------------------------------------------------------------------------------
// UI カラーパレット
//------------------------------------------------------------------------------
namespace palette {

extern const ColorRGB kBackground;   // 画面最奥
extern const ColorRGB kPanel;        // パネル地
extern const ColorRGB kPanelLight;   // パネル明部
extern const ColorRGB kPanelDark;    // パネル暗部
extern const ColorRGB kBorder;       // 枠線
extern const ColorRGB kAccent;       // アクセント（シアン）
extern const ColorRGB kAccentWarm;   // アクセント（オレンジ）
extern const ColorRGB kText;         // 標準テキスト
extern const ColorRGB kTextDim;      // 補助テキスト
extern const ColorRGB kTextDisabled; // 無効テキスト
extern const ColorRGB kHp;           // HP バー
extern const ColorRGB kHpLoss;       // HP 減少残像
extern const ColorRGB kMp;           // MP バー
extern const ColorRGB kExp;          // EXP バー
extern const ColorRGB kBossHp;       // ボス HP
extern const ColorRGB kDanger;       // 警告
extern const ColorRGB kCritical;     // クリティカル表示
extern const ColorRGB kBlack;
extern const ColorRGB kWhite;

} // namespace palette

//------------------------------------------------------------------------------
// 実行時設定（設定タブで変更）
//------------------------------------------------------------------------------
struct GameSettings
{
    int  bgmVolume = 70;
    int  seVolume = 80;
    bool showDamageNumbers = true;
    bool screenShake = true;
    bool showFps = false;
    bool fullScreen = false;
};

} // namespace ecl
