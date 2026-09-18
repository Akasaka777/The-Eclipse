//==============================================================================
// SettingsPanel.h : 設定タブ（ホーム / 戦闘中メニュー共用）
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/NamePanel.h"
#include "UI/UIWidgets.h"

#include <string>

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
    void DrawKeyGuide() const;
    void DrawSectionHeader(float x, float y, float width, const char* title) const;

    // 左カラムの範囲
    float columnLeft_ = 0.0f;
    float columnRight_ = 0.0f;

    Rect   window_;
    Button closeButton_;
    Slider bgmSlider_;
    Slider seSlider_;
    Toggle damageToggle_;
    Toggle shakeToggle_;
    Toggle fpsToggle_;
    Toggle fullScreenToggle_;
    Toggle debugToggle_;
    Toggle softwareCursorToggle_;
    Button deleteSaveButton_;
    Button logoutButton_;
    // プレイヤー名の変更（デバッグモード中のみ表示）
    Button    nameButton_;
    NamePanel namePanel_;

    // セーブ削除の確認中か
    bool   confirmingDelete_ = false;
    bool   confirmingLogout_ = false;
    std::string message_;
    float  messageTimer_ = 0.0f;

    bool open_ = false;
    bool closeRequested_ = false;
};

} // namespace ui
} // namespace ecl
