//==============================================================================
// SettingsPanel.h : 設定タブ（ホーム / 戦闘中メニュー共用）
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

namespace ecl {
namespace ui {

class SettingsPanel
{
public:
    SettingsPanel();

    void Open(const GameSettings& settings);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    // 設定の変更を ctx に反映する
    void Update(float dt, const Input& input, GameContext& context);
    void Draw() const;

    bool CloseRequested() const { return closeRequested_; }

private:
    void Layout();

    Rect   window_;
    Button closeButton_;
    Slider bgmSlider_;
    Slider seSlider_;
    Toggle damageToggle_;
    Toggle shakeToggle_;
    Toggle fpsToggle_;
    Toggle fullScreenToggle_;

    bool open_ = false;
    bool closeRequested_ = false;
};

} // namespace ui
} // namespace ecl
