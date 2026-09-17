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
#include "UI/EquipPanel.h"
#include "UI/QuestPanel.h"
#include "UI/SettingsPanel.h"
#include "UI/SkillPanel.h"
#include "UI/UIWidgets.h"
#include "UI/SmithPanel.h"

#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 画面下部のタブ
//------------------------------------------------------------------------------
enum class HomeTab
{
    None,
    Equipment, // 装備
    Skill,     // スキルツリー
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

    Stage        stage_;
    Player       player_;
    Camera       camera_;
    CombatSystem combat_;

    std::vector<ui::Button> tabButtons_;
    HomeTab activeTab_ = HomeTab::None;

    ui::EquipPanel    equipPanel_;
    ui::SkillPanel    skillPanel_;
    ui::QuestPanel    questPanel_;
    ui::SmithPanel    smithPanel_;
    ui::SettingsPanel settingsPanel_;

    float time_ = 0.0f;
    float saveNoticeTimer_ = 0.0f;
    bool  startQuest_ = false;
};

} // namespace ecl
