//==============================================================================
// HomeScene.h : ホーム（拠点の街）
//   横スクロールの街を歩いて、自宅・ショップ・鍛冶屋・ダンジョン入口に入れる（HomeTown.h）。
//   画面下部のタブからも同じ画面を開ける。
//==============================================================================
#pragma once

#include "Core/Camera.h"
#include "Core/Scene.h"
#include "Game/Combat.h"
#include "Game/Player.h"
#include "Game/Stage.h"
#include "UI/PlayerPanel.h"
#include "UI/QuestPanel.h"
#include "UI/RoguelikePanel.h"
#include "UI/SettingsPanel.h"
#include "UI/UIWidgets.h"
#include "UI/ShopPanel.h"
#include "UI/SmithPanel.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 画面下部のタブ
//------------------------------------------------------------------------------
enum class HomeTab
{
    None,
    Player,    // プレイヤー（ステータス / 装備 / アイテム / スキル）
    Quest,     // クエスト選択
    Shop,      // ショップ（武器 / 装備 / アイテム / アクセサリー）
    Smith,     // 鍛冶屋（強化・修理）
    Settings,  // 設定
    Roguelike  // 迷宮の門（ローグライクモード。下のタブは無く、街の建物から入る）
};

//------------------------------------------------------------------------------
// 開発者モードのプルダウン（同時に開くのは 1 つだけ）
//------------------------------------------------------------------------------
enum class DebugMenu
{
    None,
    Col,     // 追加する col の量を選ぶ
    Level,   // 上げるレベルの量を選ぶ
    Unique   // 解放するユニークスキルを選ぶ
};

class HomeScene : public Scene
{
public:
    HomeScene();

    void OnEnter(GameContext& context) override;
    void Update(float dt, GameContext& context, SceneManager& manager) override;
    void Draw(GameContext& context) override;

private:
    void BuildField();
    void OpenTab(HomeTab tab, GameContext& context);
    void CloseAllTabs();
    bool AnyPanelOpen() const;

    void DrawField(const GameContext& context);
    // プレイヤーより手前に描く背景物（案内看板など）
    void DrawFieldForeground(const GameContext& context);
    void DrawPlayerSummary(const GameContext& context) const;
    void DrawTabBar(const GameContext& context) const;
    void DrawFieldGuide(const GameContext& context) const;
    // 開発者モード用のボタン群（デバッグモードが ON の時だけ表示）
    void BuildDebugButtons();
    void UpdateDebugButtons(float dt, const Input& input, GameContext& context);
    void DrawDebugPanel(const GameContext& context) const;

    Stage        stage_;
    Player       player_;
    Camera       camera_;
    CombatSystem combat_;

    std::vector<ui::Button> tabButtons_;
    std::vector<ui::Button> debugButtons_;
    // プルダウン（開いているのは常に 1 つだけ）
    std::vector<ui::Button> debugColMenu_;
    std::vector<ui::Button> debugLevelMenu_;
    std::vector<ui::Button> debugUniqueMenu_;
    DebugMenu debugMenuOpen_ = DebugMenu::None;
    HomeTab activeTab_ = HomeTab::None;

    ui::PlayerPanel   playerPanel_;
    ui::QuestPanel    questPanel_;
    ui::ShopPanel     shopPanel_;
    ui::SmithPanel    smithPanel_;
    ui::SettingsPanel settingsPanel_;
    ui::RoguelikePanel roguelikePanel_;

    float time_ = 0.0f;
    float saveNoticeTimer_ = 0.0f;
    bool  startQuest_ = false;
    bool  startRoguelike_ = false;   // 迷宮の門で「挑戦する」を押した（ランを始めて出撃する）
    // 入口の前に立っている建物（town::BuildingAt の番号。いなければ -1）
    int   nearBuilding_ = -1;

    std::string debugMessage_;
    float debugMessageTimer_ = 0.0f;
};

} // namespace ecl
