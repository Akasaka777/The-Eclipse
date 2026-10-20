//==============================================================================
// Input.h : キーボード＋マウス入力（押下 / 押した瞬間 / 離した瞬間）
//==============================================================================
#pragma once

#include "Core/KeyConfig.h"

#include <string>
#include <vector>

namespace ecl {

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

    // アクションに割り当てられたキー名（操作説明用。「A / ←」など）
    const char* ActionKeyName(GameAction action) const;
    // 1 つ目に割り当てたキーの名前（HUD のキー表示用）
    std::string PrimaryKeyName(GameAction action) const;

    // --- キー設定 -------------------------------------------------------------
    // キー配置を反映する（内容が変わったときだけ作り直す）
    void ApplyKeyConfig(const KeyConfig& config);
    const KeyConfig& GetKeyConfig() const { return keyConfig_; }
    // このフレームで押されたキー（KEY_INPUT_*）。無ければ -1（キー設定の入力待ちで使う）
    int  FirstPressedKey() const;

private:
    Input() = default;
    void RebuildBindings();

    struct Binding
    {
        std::vector<int> keys;
        std::vector<int> mouseBits;   // MOUSE_INPUT_* のビット
        std::string label;
    };

    Binding bindings_[kGameActionCount];
    KeyConfig keyConfig_;

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
