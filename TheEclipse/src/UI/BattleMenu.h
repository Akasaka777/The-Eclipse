//==============================================================================
// BattleMenu.h : 戦闘中の MENU（設定 / クエストリタイア）
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/SettingsPanel.h"
#include "UI/UIWidgets.h"

namespace ecl {
namespace ui {

class BattleMenu
{
public:
    BattleMenu();

    void Open();
    void Close();
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    // リタイアが確定したら true
    bool RetireConfirmed() const { return retireConfirmed_; }

private:
    Rect   window_;
    Button resumeButton_;
    Button settingsButton_;
    Button retireButton_;
    Button retireYesButton_;
    Button retireNoButton_;
    SettingsPanel settings_;

    bool open_ = false;
    bool confirmingRetire_ = false;
    bool retireConfirmed_ = false;
};

} // namespace ui
} // namespace ecl
