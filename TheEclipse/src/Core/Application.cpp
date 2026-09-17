#include "Core/Application.h"

#include "Common/MathUtil.h"
#include "Core/DxInclude.h"
#include "Core/FontManager.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/ResourceManager.h"
#include "Game/SaveData.h"

namespace ecl {

bool Application::Initialize()
{
    // --- DxLib 初期化前の設定 -------------------------------------------------
#if defined(DX_CHARCODEFORMAT_UTF8)
    // ソースコードは UTF-8 で記述している（/utf-8 オプションと対）
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
#endif
    SetOutApplicationLogValidFlag(FALSE);
    SetMainWindowText(config::kWindowTitle);
    ChangeWindowMode(TRUE);
    SetGraphMode(config::kScreenWidth, config::kScreenHeight, config::kColorDepth);
    // ウィンドウサイズを画面に合わせて縮小表示（1920x1080 の論理解像度は維持）
    SetWindowSizeChangeEnableFlag(TRUE, TRUE);
    SetAlwaysRunFlag(TRUE);
    SetWaitVSyncFlag(TRUE);
    // ウィンドウモード切り替えでグラフィックハンドルが失われないようにする
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);

    if (DxLib_Init() != 0) return false;

    SetDrawScreen(DX_SCREEN_BACK);
    SetBackgroundColor(8, 10, 18);

    FontManager::Instance().Initialize();
    Input::Instance().Initialize();
    math::SeedRandom(static_cast<unsigned int>(GetNowCount()));

    previousTime_ = GetNowCount();
    sceneManager_.Initialize(SceneId::Title, context_);
    return true;
}

float Application::CalcDeltaTime()
{
    const int now = GetNowCount();
    float dt = static_cast<float>(now - previousTime_) / 1000.0f;
    previousTime_ = now;

    // 処理落ち時に当たり判定をすり抜けないよう上限を設ける
    if (dt > config::kMaxDeltaTime) dt = config::kMaxDeltaTime;
    if (dt < 0.0f) dt = 0.0f;
    return dt;
}

void Application::ApplyDisplaySettings()
{
    if (context_.settings.fullScreen == fullScreenApplied_) return;

    fullScreenApplied_ = context_.settings.fullScreen;
    ChangeWindowMode(fullScreenApplied_ ? FALSE : TRUE);
    SetDrawScreen(DX_SCREEN_BACK);
}

void Application::Run()
{
    while (ProcessMessage() == 0) {
        const float dt = CalcDeltaTime();

        Input::Instance().Update();
        context_.globalTime += dt;

        sceneManager_.Update(dt, context_);

        ClearDrawScreen();
        sceneManager_.Draw(context_);
        ScreenFlip();

        ApplyDisplaySettings();

        if (context_.quitRequested) break;
    }
}

void Application::Finalize()
{
    // 終了時にも保存しておく
    SaveSystem::Save(context_);

    ResourceManager::Instance().ReleaseAll();
    FontManager::Instance().Finalize();
    DxLib_End();
}

} // namespace ecl
