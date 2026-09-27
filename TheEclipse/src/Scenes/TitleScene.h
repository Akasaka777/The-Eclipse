//==============================================================================
// TitleScene.h : タイトル画面
//==============================================================================
#pragma once

#include "Core/Scene.h"
#include "Game/WeaponType.h"
#include "UI/NamePanel.h"
#include "UI/UIWidgets.h"
#include "UI/WeaponSelectPanel.h"

namespace ecl {

class TitleScene : public Scene
{
public:
    TitleScene();

    void OnEnter(GameContext& context) override;
    void Update(float dt, GameContext& context, SceneManager& manager) override;
    void Draw(GameContext& context) override;

private:
    ui::Button startButton_;
    ui::Button exitButton_;
    ui::NamePanel namePanel_;
    ui::WeaponSelectPanel weaponPanel_;
    float time_ = 0.0f;
    bool  exitRequested_ = false;
    bool  loadAttempted_ = false;
    bool  hasSaveData_ = false;
    // 初回起動の名前入力を出したか
    bool  nameAsked_ = false;
    // 初期装備をまだ配っていない（名前と武器種を訊いてから配る）
    bool  starterPending_ = false;
    // 初回に選んだ武器種
    WeaponType chosenWeapon_ = WeaponType::OneHandSword;
    bool  weaponChosen_ = false;
};

} // namespace ecl
