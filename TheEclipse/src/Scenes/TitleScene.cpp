#include "Scenes/TitleScene.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Game/GameContext.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

namespace {
constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
} // namespace

TitleScene::TitleScene()
{
    startButton_ = ui::Button(Rect::FromXYWH(kScreenW * 0.5f - 200.0f, 700.0f, 400.0f, 78.0f),
                              "ゲームを始める", FontSize::Medium);
    exitButton_ = ui::Button(Rect::FromXYWH(kScreenW * 0.5f - 200.0f, 800.0f, 400.0f, 62.0f),
                             "終了", FontSize::Normal);
}

void TitleScene::OnEnter(GameContext& context)
{
    time_ = 0.0f;
    exitRequested_ = false;
    // 初回のみ初期装備を配る
    if (context.player.GetInventory().Items().empty()) {
        context.player.SetupNewGame();
    }
}

void TitleScene::Update(float dt, GameContext& context, SceneManager& manager)
{
    time_ += dt;
    const Input& input = Input::Instance();

    if (manager.IsTransitioning()) return;

    if (startButton_.Update(input, dt) || input.Pressed(GameAction::Confirm)) {
        manager.RequestChange(SceneId::Home);
    }
    if (exitButton_.Update(input, dt)) {
        exitRequested_ = true;
        context.quitRequested = true;
    }
}

void TitleScene::Draw(GameContext& context)
{
    (void)context;

    // --- 背景 ---------------------------------------------------------------
    draw::GradientRectV(Rect(0.0f, 0.0f, kScreenW, kScreenH), ColorRGB(6, 8, 16),
                        ColorRGB(28, 18, 44), 255, 40);

    // 星
    for (int i = 0; i < 140; ++i) {
        const float x = std::fmod(static_cast<float>(i) * 137.35f, kScreenW);
        const float y = std::fmod(static_cast<float>(i) * 71.7f, kScreenH * 0.8f);
        const float twinkle = 0.4f + 0.6f * std::sin(time_ * 1.4f + static_cast<float>(i));
        draw::Circle(x, y, 1.0f + static_cast<float>(i % 3) * 0.7f, palette::kWhite, true, 1.0f,
                     static_cast<int>(140.0f * twinkle));
    }

    // 日蝕
    const float cx = kScreenW * 0.5f;
    const float cy = 300.0f;
    draw::Glow(cx, cy, 260.0f + std::sin(time_ * 0.8f) * 14.0f, ColorRGB(210, 130, 255), 170, 7);
    draw::Circle(cx, cy, 150.0f, ColorRGB(255, 232, 210), true, 1.0f, 255);
    draw::Circle(cx, cy, 140.0f, ColorRGB(8, 6, 16), true, 1.0f, 255);

    // --- タイトル ------------------------------------------------------------
    draw::TextShadow(FontSize::Title, cx, 470.0f, palette::kText, "The Eclipse",
                     draw::TextAlign::Center);
    draw::Text(FontSize::Normal, cx, 580.0f, palette::kAccent,
               "― 蝕の夜に、剣を執れ ―", draw::TextAlign::Center);

    // 装飾ライン
    draw::Line(cx - 340.0f, 560.0f, cx + 340.0f, 560.0f, palette::kAccent, 2.0f, 160);

    startButton_.Draw();
    exitButton_.Draw();

    draw::Text(FontSize::Tiny, kScreenW * 0.5f, kScreenH - 60.0f, palette::kTextDim,
               "ENTER / クリックで開始   横スクロール 2D アクション RPG プロトタイプ",
               draw::TextAlign::Center);
    draw::Text(FontSize::Tiny, kScreenW - 24.0f, kScreenH - 32.0f, palette::kTextDisabled,
               "DxLib / C++17", draw::TextAlign::Right);
}

} // namespace ecl
