#include "Core/Input.h"

#include "Core/DxInclude.h"

namespace ecl {

namespace {
int MouseBit(MouseButton button)
{
    switch (button) {
    case MouseButton::Left:   return MOUSE_INPUT_LEFT;
    case MouseButton::Right:  return MOUSE_INPUT_RIGHT;
    case MouseButton::Middle: return MOUSE_INPUT_MIDDLE;
    default: return 0;
    }
}
} // namespace

Input& Input::Instance()
{
    static Input instance;
    return instance;
}

void Input::Initialize()
{
    auto bind = [this](GameAction action, std::vector<int> keys, int mouseButton, const char* label) {
        Binding& b = bindings_[static_cast<int>(action)];
        b.keys = std::move(keys);
        b.mouseButton = mouseButton;
        b.label = label;
    };

    bind(GameAction::MoveLeft,  { KEY_INPUT_A, KEY_INPUT_LEFT },  -1, "A / ←");
    bind(GameAction::MoveRight, { KEY_INPUT_D, KEY_INPUT_RIGHT }, -1, "D / →");
    bind(GameAction::MoveUp,    { KEY_INPUT_W, KEY_INPUT_UP },    -1, "W / ↑");
    bind(GameAction::MoveDown,  { KEY_INPUT_S, KEY_INPUT_DOWN },  -1, "S / ↓");
    // 上下は奥行き移動に使うため、ジャンプは SPACE のみ
    bind(GameAction::Jump,      { KEY_INPUT_SPACE }, -1, "SPACE");
    bind(GameAction::Attack,    { KEY_INPUT_J },  MOUSE_INPUT_LEFT,  "左クリック / J");
    bind(GameAction::Guard,     { KEY_INPUT_K },  MOUSE_INPUT_RIGHT, "右クリック / K");
    bind(GameAction::Dash,      { KEY_INPUT_LSHIFT, KEY_INPUT_RSHIFT }, -1, "SHIFT");
    bind(GameAction::Skill1,    { KEY_INPUT_1, KEY_INPUT_NUMPAD1 }, -1, "1");
    bind(GameAction::Skill2,    { KEY_INPUT_2, KEY_INPUT_NUMPAD2 }, -1, "2");
    bind(GameAction::Skill3,    { KEY_INPUT_3, KEY_INPUT_NUMPAD3 }, -1, "3");
    bind(GameAction::Skill4,    { KEY_INPUT_4, KEY_INPUT_NUMPAD4 }, -1, "4");
    bind(GameAction::Menu,      { KEY_INPUT_ESCAPE }, -1, "ESC");
    bind(GameAction::Confirm,   { KEY_INPUT_RETURN }, -1, "ENTER");
    bind(GameAction::Cancel,    { KEY_INPUT_ESCAPE, KEY_INPUT_BACK }, -1, "ESC");

    GetHitKeyStateAll(currentKeys_);
    for (int i = 0; i < 256; ++i) previousKeys_[i] = currentKeys_[i];
    GetMousePoint(&mouseX_, &mouseY_);
    prevMouseX_ = mouseX_;
    prevMouseY_ = mouseY_;
}

void Input::Update()
{
    for (int i = 0; i < 256; ++i) previousKeys_[i] = currentKeys_[i];
    GetHitKeyStateAll(currentKeys_);

    prevMouseX_ = mouseX_;
    prevMouseY_ = mouseY_;
    GetMousePoint(&mouseX_, &mouseY_);
    mouseMoved_ = (prevMouseX_ != mouseX_) || (prevMouseY_ != mouseY_);

    prevMouseState_ = mouseState_;
    mouseState_ = GetMouseInput();

    wheelDelta_ = GetMouseWheelRotVol();
}

bool Input::Down(GameAction action) const
{
    const Binding& b = bindings_[static_cast<int>(action)];
    for (int key : b.keys) {
        if (KeyDown(key)) return true;
    }
    if (b.mouseButton >= 0 && (mouseState_ & b.mouseButton) != 0) return true;
    return false;
}

bool Input::Pressed(GameAction action) const
{
    const Binding& b = bindings_[static_cast<int>(action)];
    for (int key : b.keys) {
        if (KeyPressed(key)) return true;
    }
    if (b.mouseButton >= 0) {
        const bool now = (mouseState_ & b.mouseButton) != 0;
        const bool before = (prevMouseState_ & b.mouseButton) != 0;
        if (now && !before) return true;
    }
    return false;
}

bool Input::Released(GameAction action) const
{
    const Binding& b = bindings_[static_cast<int>(action)];
    for (int key : b.keys) {
        if (key >= 0 && key < 256 && previousKeys_[key] != 0 && currentKeys_[key] == 0) return true;
    }
    if (b.mouseButton >= 0) {
        const bool now = (mouseState_ & b.mouseButton) != 0;
        const bool before = (prevMouseState_ & b.mouseButton) != 0;
        if (!now && before) return true;
    }
    return false;
}

bool Input::KeyDown(int keyCode) const
{
    if (keyCode < 0 || keyCode >= 256) return false;
    return currentKeys_[keyCode] != 0;
}

bool Input::KeyPressed(int keyCode) const
{
    if (keyCode < 0 || keyCode >= 256) return false;
    return currentKeys_[keyCode] != 0 && previousKeys_[keyCode] == 0;
}

bool Input::MouseDown(MouseButton button) const
{
    return (mouseState_ & MouseBit(button)) != 0;
}

bool Input::MouseClicked(MouseButton button) const
{
    const int bit = MouseBit(button);
    return (mouseState_ & bit) != 0 && (prevMouseState_ & bit) == 0;
}

bool Input::MouseReleased(MouseButton button) const
{
    const int bit = MouseBit(button);
    return (mouseState_ & bit) == 0 && (prevMouseState_ & bit) != 0;
}

float Input::MoveAxisX() const
{
    float axis = 0.0f;
    if (Down(GameAction::MoveLeft))  axis -= 1.0f;
    if (Down(GameAction::MoveRight)) axis += 1.0f;
    return axis;
}

const char* Input::ActionKeyName(GameAction action) const
{
    return bindings_[static_cast<int>(action)].label;
}

} // namespace ecl
