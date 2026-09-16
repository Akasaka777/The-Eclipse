//==============================================================================
// Input.h : キーボード＋マウス入力（押下 / 押した瞬間 / 離した瞬間）
//==============================================================================
#pragma once

#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// ゲーム内アクション（キーコンフィグの単位）
//------------------------------------------------------------------------------
enum class GameAction
{
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    Jump,
    Attack,
    Guard,
    Dash,
    Skill1,
    Skill2,
    Skill3,
    Skill4,
    Menu,
    Confirm,
    Cancel,
    Count
};

enum class MouseButton
{
    Left = 0,
    Right = 1,
    Middle = 2,
    Count
};

//------------------------------------------------------------------------------
// 入力シングルトン
//------------------------------------------------------------------------------
class Input
{
public:
    static Input& Instance();

    void Initialize();
    void Update();

    // アクション
    bool Down(GameAction action) const;
    bool Pressed(GameAction action) const;
    bool Released(GameAction action) const;

    // 生キー（KEY_INPUT_*）
    bool KeyDown(int keyCode) const;
    bool KeyPressed(int keyCode) const;

    // マウス
    int  MouseX() const { return mouseX_; }
    int  MouseY() const { return mouseY_; }
    bool MouseDown(MouseButton button) const;
    bool MouseClicked(MouseButton button) const;
    bool MouseReleased(MouseButton button) const;
    int  WheelDelta() const { return wheelDelta_; }
    bool MouseMoved() const { return mouseMoved_; }

    // 左右移動の軸（-1 / 0 / +1）
    float MoveAxisX() const;

    // アクションに割り当てられたキー名（操作説明用）
    const char* ActionKeyName(GameAction action) const;

private:
    Input() = default;

    struct Binding
    {
        std::vector<int> keys;
        int mouseButton = -1; // -1 なら未割り当て
        const char* label = "";
    };

    Binding bindings_[static_cast<int>(GameAction::Count)];

    char currentKeys_[256] = {};
    char previousKeys_[256] = {};

    int  mouseX_ = 0;
    int  mouseY_ = 0;
    int  prevMouseX_ = 0;
    int  prevMouseY_ = 0;
    int  mouseState_ = 0;
    int  prevMouseState_ = 0;
    int  wheelDelta_ = 0;
    bool mouseMoved_ = false;
};

} // namespace ecl
