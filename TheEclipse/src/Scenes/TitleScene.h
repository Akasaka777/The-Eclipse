//==============================================================================
// TitleScene.h : タイトル画面
//==============================================================================
#pragma once

#include "Core/Scene.h"
#include "UI/NamePanel.h"
#include "UI/UIWidgets.h"

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
    float time_ = 0.0f;
    bool  exitRequested_ = false;
    bool  loadAttempted_ = false;
    bool  hasSaveData_ = false;
    // 初回起動の名前入力を出したか
    bool  nameAsked_ = false;
};

} // namespace ecl
