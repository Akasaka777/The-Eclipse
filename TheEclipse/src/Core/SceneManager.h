//==============================================================================
// SceneManager.h : シーンの生成・切り替え・フェード
//==============================================================================
#pragma once

#include "Core/Scene.h"

#include <memory>

namespace ecl {

class SceneManager
{
public:
    void Initialize(SceneId first, GameContext& context);
    // 次のシーンを予約（フェードを挟んで切り替わる）
    void RequestChange(SceneId next);

    void Update(float dt, GameContext& context);
    void Draw(GameContext& context);

    SceneId CurrentId() const { return currentId_; }
    bool IsTransitioning() const { return fadeState_ != FadeState::None; }

private:
    enum class FadeState
    {
        None,
        Out,
        In
    };

    static std::unique_ptr<Scene> CreateScene(SceneId id);
    void SwitchTo(SceneId id, GameContext& context);

    std::unique_ptr<Scene> current_;
    SceneId currentId_ = SceneId::None;
    SceneId pending_ = SceneId::None;
    FadeState fadeState_ = FadeState::None;
    float fadeAlpha_ = 0.0f;
};

} // namespace ecl
