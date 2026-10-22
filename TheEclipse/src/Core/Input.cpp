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
    case MouseButton::SideBack:    return MOUSE_INPUT_4;
    case MouseButton::SideForward: return MOUSE_INPUT_5;
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
    // 初期のキー配置（セーブデータの設定は GameContext から ApplyKeyConfig で反映する）
    keyConfig_ = KeyConfig();
    RebuildBindings();

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
#if defined(_WIN32)
    // マウスのサイドボタン（戻る / 進む）は環境によって GetMouseInput に乗らないことがあるため、
    // ウィンドウが前面にあるときだけ OS からも直接読む
    if (GetForegroundWindow() == GetMainWindowHandle()) {
        if ((GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0) mouseState_ |= MOUSE_INPUT_4;
        if ((GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0) mouseState_ |= MOUSE_INPUT_5;
    }
#endif

    wheelDelta_ = GetMouseWheelRotVol();
}

void Input::ApplyKeyConfig(const KeyConfig& config)
{
    if (config == keyConfig_) return;
    keyConfig_ = config;
    RebuildBindings();
}

void Input::RebuildBindings()
{
    for (int i = 0; i < kGameActionCount; ++i) {
        const GameAction action = static_cast<GameAction>(i);
        Binding& b = bindings_[i];
        b.keys.clear();
        b.mouseBits.clear();
        for (int s = 0; s < kKeySlotsPerAction; ++s) {
            const int code = keyConfig_.Get(action, s);
            if (code == kNoInput) continue;
            if (IsMouseInputCode(code)) {
                b.mouseBits.push_back(MouseBit(MouseButtonOfCode(code)));
            } else {
                b.keys.push_back(code);
            }
        }
        b.label = keyConfig_.Describe(action);
    }
}

bool Input::Down(GameAction action) const
{
    const Binding& b = bindings_[static_cast<int>(action)];
    for (int key : b.keys) {
        if (KeyDown(key)) return true;
    }
    for (int bit : b.mouseBits) {
        if ((mouseState_ & bit) != 0) return true;
    }
    return false;
}

bool Input::Pressed(GameAction action) const
{
    const Binding& b = bindings_[static_cast<int>(action)];
    for (int key : b.keys) {
        if (KeyPressed(key)) return true;
    }
    for (int bit : b.mouseBits) {
        const bool now = (mouseState_ & bit) != 0;
        const bool before = (prevMouseState_ & bit) != 0;
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
    for (int bit : b.mouseBits) {
        const bool now = (mouseState_ & bit) != 0;
        const bool before = (prevMouseState_ & bit) != 0;
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
    return bindings_[static_cast<int>(action)].label.c_str();
}

std::string Input::PrimaryKeyName(GameAction action) const
{
    return keyConfig_.PrimaryName(action);
}

int Input::FirstPressedKey() const
{
    for (int i = 0; i < 256; ++i) {
        if (KeyPressed(i)) return i;
    }
    return -1;
}

} // namespace ecl
