//==============================================================================
// KeyConfigPanel.h : キー設定（設定タブから開く）
//   アクションごとに 2 つの枠があり、枠をクリックしてから押したキー
//   （またはマウスのボタン）がその枠に割り当てられる。
//   ほかのアクションで使っているキーを選ぶと、そのアクションと入れ替える。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Core/KeyConfig.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

class KeyConfigPanel
{
public:
    KeyConfigPanel();

    void Open();
    void Close() { open_ = false; waitingAction_ = -1; }
    bool IsOpen() const { return open_; }

    // 変更は context.settings.keys に書き込み、すぐ入力にも反映する
    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    // --- 検証用 ---------------------------------------------------------------
    const Rect& WindowRect() const { return window_; }
    // 変更できるアクションの並び（行の順）
    const std::vector<GameAction>& Actions() const { return actions_; }
    const Rect& SlotRect(int row, int slot) const;
    const Rect& ResetRect() const { return resetButton_.GetRect(); }
    const Rect& CloseRect() const { return closeButton_.GetRect(); }
    // 入力待ちの行と枠（待っていなければ -1）
    int  WaitingRow() const { return waitingAction_; }
    int  WaitingSlot() const { return waitingSlot_; }
    const std::string& Message() const { return message_; }

private:
    void Layout();
    // 入力待ちの処理（何か割り当てた / 取り消したら true）
    bool UpdateWaiting(const Input& input, GameContext& context);
    void AssignTo(int code, GameContext& context);
    void ShowMessage(const std::string& text, bool warning);

    Rect window_;
    std::vector<GameAction> actions_;
    std::vector<Button> slotButtons_;   // 行 × 枠
    Button resetButton_;
    Button closeButton_;

    int  waitingAction_ = -1;   // 入力待ちの行
    int  waitingSlot_ = -1;
    bool open_ = false;

    std::string message_;
    bool  messageWarning_ = false;
    float messageTimer_ = 0.0f;
    float time_ = 0.0f;
};

} // namespace ui
} // namespace ecl
