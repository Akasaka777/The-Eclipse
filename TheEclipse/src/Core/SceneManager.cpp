#include "Core/SceneManager.h"

#include "Common/MathUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"
#include "Scenes/HomeScene.h"
#include "Scenes/QuestScene.h"
#include "Scenes/ResultScene.h"
#include "Scenes/TitleScene.h"

namespace ecl {

namespace {
constexpr float kFadeSpeed = 3.2f;
} // namespace

std::unique_ptr<Scene> SceneManager::CreateScene(SceneId id)
{
    switch (id) {
    case SceneId::Title:  return std::unique_ptr<Scene>(new TitleScene());
    case SceneId::Home:   return std::unique_ptr<Scene>(new HomeScene());
    case SceneId::Quest:  return std::unique_ptr<Scene>(new QuestScene());
    case SceneId::Result: return std::unique_ptr<Scene>(new ResultScene());
    default: return nullptr;
    }
}

void SceneManager::Initialize(SceneId first, GameContext& context)
{
    SwitchTo(first, context);
    fadeState_ = FadeState::In;
    fadeAlpha_ = 1.0f;
}

void SceneManager::SwitchTo(SceneId id, GameContext& context)
{
    if (current_) current_->OnExit(context);
    current_ = CreateScene(id);
    currentId_ = id;
    if (current_) current_->OnEnter(context);
}

void SceneManager::RequestChange(SceneId next)
{
    if (fadeState_ == FadeState::Out) return;
    pending_ = next;
    fadeState_ = FadeState::Out;
}

void SceneManager::Update(float dt, GameContext& context)
{
    switch (fadeState_) {
    case FadeState::Out:
        fadeAlpha_ = math::Approach(fadeAlpha_, 1.0f, dt * kFadeSpeed);
        if (fadeAlpha_ >= 1.0f) {
            SwitchTo(pending_, context);
            pending_ = SceneId::None;
            fadeState_ = FadeState::In;
        }
        break;
    case FadeState::In:
        fadeAlpha_ = math::Approach(fadeAlpha_, 0.0f, dt * kFadeSpeed);
        if (fadeAlpha_ <= 0.0f) fadeState_ = FadeState::None;
        break;
    default:
        break;
    }

    if (current_) current_->Update(dt, context, *this);
}

void SceneManager::Draw(GameContext& context)
{
    if (current_) current_->Draw(context);

    if (fadeAlpha_ > 0.001f) {
        draw::FillRect(Rect(0.0f, 0.0f, static_cast<float>(config::kScreenWidth),
                            static_cast<float>(config::kScreenHeight)),
                       palette::kBlack, static_cast<int>(fadeAlpha_ * 255.0f));
    }
}

} // namespace ecl
