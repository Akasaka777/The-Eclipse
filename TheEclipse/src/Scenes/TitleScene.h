//==============================================================================
// TitleScene.h : タイトル画面
//==============================================================================
#pragma once

#include "Core/Scene.h"
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
    float time_ = 0.0f;
    bool  exitRequested_ = false;
    bool  loadAttempted_ = false;
    bool  hasSaveData_ = false;
};

} // namespace ecl
