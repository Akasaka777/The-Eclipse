//==============================================================================
// main.cpp : The Eclipse エントリポイント
//   横スクロール 2D アクション RPG プロトタイプ（DxLib / Visual Studio 2022）
//==============================================================================
#include "Core/Application.h"
#include "Core/DxInclude.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    ecl::Application application;
    if (!application.Initialize()) {
        return -1;
    }

    application.Run();
    application.Finalize();
    return 0;
}
