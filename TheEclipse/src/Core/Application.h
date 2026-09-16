//==============================================================================
// Application.h : DxLib の初期化とメインループ
//==============================================================================
#pragma once

#include "Core/SceneManager.h"
#include "Game/GameContext.h"

namespace ecl {

class Application
{
public:
    // DxLib_Init までを行う（失敗したら false）
    bool Initialize();
    void Run();
    void Finalize();

private:
    float CalcDeltaTime();
    void  ApplyDisplaySettings();

    GameContext  context_;
    SceneManager sceneManager_;
    int   previousTime_ = 0;
    bool  fullScreenApplied_ = false;
};

} // namespace ecl
