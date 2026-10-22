#include "Core/KeyConfig.h"

#include "Core/DxInclude.h"

namespace ecl {

namespace {

// 割り当てられるキーと表示名
struct KeyName
{
    int         code;
    const char* name;
};

const KeyName kKeyNames[] = {
    { KEY_INPUT_A, "A" }, { KEY_INPUT_B, "B" }, { KEY_INPUT_C, "C" }, { KEY_INPUT_D, "D" },
    { KEY_INPUT_E, "E" }, { KEY_INPUT_F, "F" }, { KEY_INPUT_G, "G" }, { KEY_INPUT_H, "H" },
    { KEY_INPUT_I, "I" }, { KEY_INPUT_J, "J" }, { KEY_INPUT_K, "K" }, { KEY_INPUT_L, "L" },
    { KEY_INPUT_M, "M" }, { KEY_INPUT_N, "N" }, { KEY_INPUT_O, "O" }, { KEY_INPUT_P, "P" },
    { KEY_INPUT_Q, "Q" }, { KEY_INPUT_R, "R" }, { KEY_INPUT_S, "S" }, { KEY_INPUT_T, "T" },
    { KEY_INPUT_U, "U" }, { KEY_INPUT_V, "V" }, { KEY_INPUT_W, "W" }, { KEY_INPUT_X, "X" },
    { KEY_INPUT_Y, "Y" }, { KEY_INPUT_Z, "Z" },
    { KEY_INPUT_0, "0" }, { KEY_INPUT_1, "1" }, { KEY_INPUT_2, "2" }, { KEY_INPUT_3, "3" },
    { KEY_INPUT_4, "4" }, { KEY_INPUT_5, "5" }, { KEY_INPUT_6, "6" }, { KEY_INPUT_7, "7" },
    { KEY_INPUT_8, "8" }, { KEY_INPUT_9, "9" },
    { KEY_INPUT_NUMPAD0, "テンキー0" }, { KEY_INPUT_NUMPAD1, "テンキー1" },
    { KEY_INPUT_NUMPAD2, "テンキー2" }, { KEY_INPUT_NUMPAD3, "テンキー3" },
    { KEY_INPUT_NUMPAD4, "テンキー4" }, { KEY_INPUT_NUMPAD5, "テンキー5" },
    { KEY_INPUT_NUMPAD6, "テンキー6" }, { KEY_INPUT_NUMPAD7, "テンキー7" },
    { KEY_INPUT_NUMPAD8, "テンキー8" }, { KEY_INPUT_NUMPAD9, "テンキー9" },
    { KEY_INPUT_LEFT, "←" }, { KEY_INPUT_RIGHT, "→" }, { KEY_INPUT_UP, "↑" }, { KEY_INPUT_DOWN, "↓" },
    { KEY_INPUT_SPACE, "SPACE" }, { KEY_INPUT_RETURN, "ENTER" }, { KEY_INPUT_TAB, "TAB" },
    { KEY_INPUT_LSHIFT, "左SHIFT" }, { KEY_INPUT_RSHIFT, "右SHIFT" },
    { KEY_INPUT_LCONTROL, "左CTRL" }, { KEY_INPUT_RCONTROL, "右CTRL" },
    { KEY_INPUT_LALT, "左ALT" }, { KEY_INPUT_RALT, "右ALT" },
    { KEY_INPUT_MINUS, "-" }, { KEY_INPUT_PREVTRACK, "^" }, { KEY_INPUT_YEN, "￥" },
    { KEY_INPUT_AT, "@" }, { KEY_INPUT_LBRACKET, "[" }, { KEY_INPUT_RBRACKET, "]" },
    { KEY_INPUT_SEMICOLON, ";" }, { KEY_INPUT_COLON, ":" }, { KEY_INPUT_COMMA, "," },
    { KEY_INPUT_PERIOD, "." }, { KEY_INPUT_SLASH, "/" }, { KEY_INPUT_BACKSLASH, "＼" },
};

struct ActionInfo
{
    const char* label;
    const char* id;
    bool        configurable;
    int         defaults[kKeySlotsPerAction];
};

// GameAction の並び順と同じ
const ActionInfo kActions[kGameActionCount] = {
    { "左へ移動",       "moveleft",   true,  { KEY_INPUT_A, KEY_INPUT_LEFT } },
    { "右へ移動",       "moveright",  true,  { KEY_INPUT_D, KEY_INPUT_RIGHT } },
    { "奥へ移動",       "moveup",     true,  { KEY_INPUT_W, KEY_INPUT_UP } },
    { "手前へ移動",     "movedown",   true,  { KEY_INPUT_S, KEY_INPUT_DOWN } },
    // 上下は奥行き移動に使うため、ジャンプは SPACE のみ
    { "ジャンプ",       "jump",       true,  { KEY_INPUT_SPACE, kNoInput } },
    { "通常攻撃",       "attack",     true,  { kMouseInputBase + 0, KEY_INPUT_J } },
    { "ガード",         "guard",      true,  { kMouseInputBase + 1, KEY_INPUT_K } },
    { "回避ダッシュ",   "dash",       true,  { KEY_INPUT_LSHIFT, KEY_INPUT_RSHIFT } },
    { "ソードスキル 1", "skill1",     true,  { KEY_INPUT_1, KEY_INPUT_NUMPAD1 } },
    { "ソードスキル 2", "skill2",     true,  { KEY_INPUT_2, KEY_INPUT_NUMPAD2 } },
    { "ソードスキル 3", "skill3",     true,  { KEY_INPUT_3, KEY_INPUT_NUMPAD3 } },
    { "ソードスキル 4", "skill4",     true,  { KEY_INPUT_4, KEY_INPUT_NUMPAD4 } },
    { "アイテム切替",   "itemswitch", true,  { KEY_INPUT_Q, kNoInput } },
    { "アイテム使用",   "itemuse",    true,  { KEY_INPUT_E, kNoInput } },
    { "メニュー",       "menu",       false, { KEY_INPUT_ESCAPE, kNoInput } },
    { "決定",           "confirm",    false, { KEY_INPUT_RETURN, kNoInput } },
    { "取り消し",       "cancel",     false, { KEY_INPUT_ESCAPE, KEY_INPUT_BACK } },
};

int ActionIndex(GameAction action)
{
    const int index = static_cast<int>(action);
    return (index >= 0 && index < kGameActionCount) ? index : -1;
}

bool ValidSlot(int slot)
{
    return slot >= 0 && slot < kKeySlotsPerAction;
}

} // namespace

int MouseInputCode(MouseButton button)
{
    return kMouseInputBase + static_cast<int>(button);
}

bool IsMouseInputCode(int code)
{
    return code >= kMouseInputBase && code < kMouseInputBase + static_cast<int>(MouseButton::Count);
}

MouseButton MouseButtonOfCode(int code)
{
    return IsMouseInputCode(code) ? static_cast<MouseButton>(code - kMouseInputBase) : MouseButton::Count;
}

KeyConfig::KeyConfig()
{
    ResetToDefaults();
}

void KeyConfig::ResetToDefaults()
{
    for (int i = 0; i < kGameActionCount; ++i) {
        for (int s = 0; s < kKeySlotsPerAction; ++s) codes[i][s] = kActions[i].defaults[s];
    }
}

int KeyConfig::Get(GameAction action, int slot) const
{
    const int index = ActionIndex(action);
    if (index < 0 || !ValidSlot(slot)) return kNoInput;
    return codes[index][slot];
}

bool KeyConfig::HasAny(GameAction action) const
{
    for (int s = 0; s < kKeySlotsPerAction; ++s) {
        if (Get(action, s) != kNoInput) return true;
    }
    return false;
}

std::string KeyConfig::Describe(GameAction action) const
{
    std::string text;
    for (int s = 0; s < kKeySlotsPerAction; ++s) {
        const int code = Get(action, s);
        if (code == kNoInput) continue;
        if (!text.empty()) text += " / ";
        text += InputName(code);
    }
    return text.empty() ? std::string("未設定") : text;
}

std::string KeyConfig::PrimaryName(GameAction action) const
{
    for (int s = 0; s < kKeySlotsPerAction; ++s) {
        const int code = Get(action, s);
        if (code != kNoInput) return InputName(code);
    }
    return std::string();
}

std::string KeyConfig::SkillKeysSummary(int count) const
{
    const GameAction skills[4] = { GameAction::Skill1, GameAction::Skill2, GameAction::Skill3,
                                   GameAction::Skill4 };
    if (count < 1) count = 1;
    if (count > 4) count = 4;

    bool numbered = true;
    std::string joined;
    for (int i = 0; i < count; ++i) {
        const std::string name = PrimaryName(skills[i]);
        if (name != std::string(1, static_cast<char>('1' + i))) numbered = false;
        if (!joined.empty()) joined += "/";
        joined += name.empty() ? std::string("-") : name;
    }
    if (numbered) return (count == 1) ? std::string("1") : "1〜" + std::string(1, static_cast<char>('0' + count));
    return joined;
}

GameAction KeyConfig::Assign(GameAction action, int slot, int code)
{
    const int index = ActionIndex(action);
    if (index < 0 || !ValidSlot(slot) || !IsConfigurable(action) || !IsAssignable(code)) {
        return GameAction::Count;
    }
    const int previous = codes[index][slot];
    if (previous == code) return GameAction::Count;

    // 同じ入力を使っている枠と入れ替える
    for (int i = 0; i < kGameActionCount; ++i) {
        if (!kActions[i].configurable) continue;
        for (int s = 0; s < kKeySlotsPerAction; ++s) {
            if (i == index && s == slot) continue;
            if (codes[i][s] != code) continue;
            codes[i][s] = previous;
            codes[index][slot] = code;
            return static_cast<GameAction>(i);
        }
    }
    codes[index][slot] = code;
    return GameAction::Count;
}

bool KeyConfig::Clear(GameAction action, int slot)
{
    const int index = ActionIndex(action);
    if (index < 0 || !ValidSlot(slot) || !IsConfigurable(action)) return false;
    // 最後の 1 つは外せない（操作できなくなるため）
    int others = 0;
    for (int s = 0; s < kKeySlotsPerAction; ++s) {
        if (s != slot && codes[index][s] != kNoInput) ++others;
    }
    if (others == 0) return false;
    codes[index][slot] = kNoInput;
    return true;
}

bool KeyConfig::operator==(const KeyConfig& other) const
{
    for (int i = 0; i < kGameActionCount; ++i) {
        for (int s = 0; s < kKeySlotsPerAction; ++s) {
            if (codes[i][s] != other.codes[i][s]) return false;
        }
    }
    return true;
}

bool KeyConfig::IsConfigurable(GameAction action)
{
    const int index = ActionIndex(action);
    return index >= 0 && kActions[index].configurable;
}

const char* KeyConfig::ActionLabel(GameAction action)
{
    const int index = ActionIndex(action);
    return (index >= 0) ? kActions[index].label : "";
}

const char* KeyConfig::ActionId(GameAction action)
{
    const int index = ActionIndex(action);
    return (index >= 0) ? kActions[index].id : "";
}

GameAction KeyConfig::FindActionById(const std::string& id)
{
    for (int i = 0; i < kGameActionCount; ++i) {
        if (id == kActions[i].id) return static_cast<GameAction>(i);
    }
    return GameAction::Count;
}

bool KeyConfig::IsAssignable(int code)
{
    if (IsMouseInputCode(code)) return true;
    for (const KeyName& key : kKeyNames) {
        if (key.code == code) return true;
    }
    return false;
}

std::string KeyConfig::InputName(int code)
{
    switch (MouseButtonOfCode(code)) {
    case MouseButton::Left:   return "左クリック";
    case MouseButton::Right:  return "右クリック";
    case MouseButton::Middle: return "中クリック";
    case MouseButton::SideBack:    return "サイド(戻る)";
    case MouseButton::SideForward: return "サイド(進む)";
    default: break;
    }
    if (code == KEY_INPUT_ESCAPE) return "ESC";
    if (code == KEY_INPUT_BACK) return "BackSpace";
    for (const KeyName& key : kKeyNames) {
        if (key.code == code) return key.name;
    }
    return (code == kNoInput) ? std::string() : std::string("?");
}

} // namespace ecl
