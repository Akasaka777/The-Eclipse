//==============================================================================
// ResultScene.h : クエスト終了後のリザルト
//==============================================================================
#pragma once

#include "Core/Scene.h"
#include "UI/UIWidgets.h"

namespace ecl {

class ResultScene : public Scene
{
public:
    ResultScene();

    void OnEnter(GameContext& context) override;
    void Update(float dt, GameContext& context, SceneManager& manager) override;
    void Draw(GameContext& context) override;

private:
    void DrawHeader(const GameContext& context) const;
    void DrawStats(const GameContext& context) const;
    void DrawRewards(const GameContext& context) const;

    ui::Button homeButton_;
    float time_ = 0.0f;
    int   scroll_ = 0;
};

} // namespace ecl
