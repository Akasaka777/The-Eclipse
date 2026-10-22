//==============================================================================
// KeyConfig.h : キー配置（設定タブの「キー設定」で変更する）
//   ゲーム内アクションごとに 2 つまで入力を割り当てる。
//   入力はキーボードのキー（KEY_INPUT_*）かマウスのボタン。
//   メニュー・決定・取り消し（ESC / ENTER / BackSpace）は固定で変更できない。
//==============================================================================
#pragma once

#include <string>

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
    ItemSwitch,   // 戦闘中のアイテム枠を 回復 ⇔ バフ で切り替える
    ItemUse,      // 選んでいる枠のアイテムを使う
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
    SideBack = 3,      // サイドボタン（戻る）＝ MOUSE_INPUT_4
    SideForward = 4,   // サイドボタン（進む）＝ MOUSE_INPUT_5
    Count
};

constexpr int kGameActionCount = static_cast<int>(GameAction::Count);
// 1 つのアクションに割り当てられる入力の数
constexpr int kKeySlotsPerAction = 2;

//------------------------------------------------------------------------------
// 入力コード
//   0〜255   : キーボードのキー（KEY_INPUT_*）
//   1000〜   : マウスのボタン（1000 + MouseButton）
//   -1       : 割り当てなし
//------------------------------------------------------------------------------
constexpr int kNoInput = -1;
constexpr int kMouseInputBase = 1000;

int  MouseInputCode(MouseButton button);
bool IsMouseInputCode(int code);
MouseButton MouseButtonOfCode(int code);

//------------------------------------------------------------------------------
// キー配置
//------------------------------------------------------------------------------
struct KeyConfig
{
    // 初期設定で作る
    KeyConfig();

    void ResetToDefaults();

    int  Get(GameAction action, int slot) const;
    bool HasAny(GameAction action) const;
    // 「A / ←」のような割り当ての一覧（未設定なら「未設定」）
    std::string Describe(GameAction action) const;
    // 1 つ目の割り当ての名前（HUD のキー表示用。未設定なら空）
    std::string PrimaryName(GameAction action) const;
    // ソードスキル 1〜count のキーの要約（初期設定のままなら「1〜4」、変えていれば「Z/2/3/4」）
    std::string SkillKeysSummary(int count = 4) const;

    // code を action の slot 番目に割り当てる。
    //   ほかの枠で同じ入力を使っていれば、その枠と入れ替える（同じ入力が 2 か所に付かない）。
    //   入れ替えた相手のアクションを返す（入れ替えが無ければ GameAction::Count）。
    GameAction Assign(GameAction action, int slot, int code);
    // 枠を空にする。そのアクションの割り当てが 1 つも無くなる場合は空にせず false
    bool Clear(GameAction action, int slot);

    bool operator==(const KeyConfig& other) const;
    bool operator!=(const KeyConfig& other) const { return !(*this == other); }

    // --- アクションと入力の情報 -----------------------------------------------
    // キー設定で変更できるアクションか（メニュー・決定・取り消しは固定）
    static bool IsConfigurable(GameAction action);
    // 設定画面に出す名前（「左へ移動」など）
    static const char* ActionLabel(GameAction action);
    // セーブデータに書く名前（英小文字。並び順を変えても読めるように名前で保存する）
    static const char* ActionId(GameAction action);
    // ActionId から引く（見つからなければ GameAction::Count）
    static GameAction FindActionById(const std::string& id);
    // 割り当てられる入力か（ESC / BackSpace / F1〜F12 などは不可）
    static bool IsAssignable(int code);
    // 入力の表示名（「A」「SPACE」「左クリック」など）
    static std::string InputName(int code);

    int codes[kGameActionCount][kKeySlotsPerAction];
};

} // namespace ecl
