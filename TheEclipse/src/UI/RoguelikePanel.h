//==============================================================================
// RoguelikePanel.h : ローグライクモードの入口（ホームの街の「迷宮の門」）
//   解放前は条件（クエスト一覧のクエストをすべてクリア）と進み具合を出す。
//   解放後はルール・武器種の選択・これまでの記録・「挑戦する」ボタンを出す。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Game/WeaponType.h"
#include "UI/UIWidgets.h"

#include <vector>

namespace ecl {

class GameContext;
class Input;

namespace ui {

class RoguelikePanel
{
public:
    RoguelikePanel();

    void Open(const GameContext& context);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }
    // 「挑戦する」が押された（武器種は ChosenWeapon）
    bool StartRequested() const { return startRequested_; }
    WeaponType ChosenWeapon() const { return weapon_; }

    // --- 検証用 ---------------------------------------------------------------
    const Rect& StartRect() const { return startButton_.GetRect(); }
    const Rect& WeaponRect(int index) const { return weaponButtons_[static_cast<size_t>(index)].GetRect(); }
    bool StartEnabled() const { return startButton_.Enabled(); }

private:
    bool open_ = false;
    bool closeRequested_ = false;
    bool startRequested_ = false;
    WeaponType weapon_ = WeaponType::OneHandSword;

    Rect window_;
    std::vector<Button> weaponButtons_;
    Button startButton_;
    Button closeButton_;
};

} // namespace ui
} // namespace ecl
