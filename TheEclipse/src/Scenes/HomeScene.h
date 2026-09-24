//==============================================================================
// HomeScene.h : ホーム（拠点）
//   一定範囲のフィールドを自由に歩ける。画面下部に 4 つのタブを配置。
//==============================================================================
#pragma once

#include "Core/Camera.h"
#include "Core/Scene.h"
#include "Game/Combat.h"
#include "Game/Player.h"
#include "Game/Stage.h"
#include "UI/PlayerPanel.h"
#include "UI/QuestPanel.h"
#include "UI/SettingsPanel.h"
#include "UI/UIWidgets.h"
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
    Smith,     // 鍛冶屋（強化・修理）
    Settings   // 設定
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
    // 「ユニーク解放」のプルダウン
    std::vector<ui::Button> debugUniqueMenu_;
    bool debugUniqueMenuOpen_ = false;
    HomeTab activeTab_ = HomeTab::None;

    ui::PlayerPanel   playerPanel_;
    ui::QuestPanel    questPanel_;
    ui::SmithPanel    smithPanel_;
    ui::SettingsPanel settingsPanel_;

    float time_ = 0.0f;
    float saveNoticeTimer_ = 0.0f;
    bool  startQuest_ = false;

    std::string debugMessage_;
    float debugMessageTimer_ = 0.0f;
};

} // namespace ecl
