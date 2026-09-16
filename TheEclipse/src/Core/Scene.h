//==============================================================================
// Scene.h : シーンの基底
//==============================================================================
#pragma once

namespace ecl {

class GameContext;
class SceneManager;

enum class SceneId
{
    None,
    Title,
    Home,
    Quest,
    Result
};

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void OnEnter(GameContext& context) { (void)context; }
    virtual void OnExit(GameContext& context) { (void)context; }
    virtual void Update(float dt, GameContext& context, SceneManager& manager) = 0;
    virtual void Draw(GameContext& context) = 0;
};

} // namespace ecl
