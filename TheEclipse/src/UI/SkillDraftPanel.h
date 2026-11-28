//==============================================================================
// SkillDraftPanel.h : スキルを拾ったときの選択画面（ローグライクモード）
//   3 つの候補から 1 つを選ぶ。
//   ・新しいスキル：空いているスロットにセットする。4 つとも埋まっていれば、
//     入れ替えるスキルを選ぶ（入れ替えたスキルは捨てる）
//   ・持っているスキル：Lv が 1 上がる（Lv 5 まで）
//   「見送る」で何も取らずに閉じられる。開いている間はゲームが止まる。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {

class Input;
class PlayerData;

namespace ui {

class SkillDraftPanel
{
public:
    SkillDraftPanel();

    void Open(const std::vector<int>& choices);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, PlayerData& data);
    void Draw(const PlayerData& data) const;

    // 閉じたあとに見る：スキル構成が変わったか・何が起きたかの一言
    bool Changed() const { return changed_; }
    const std::string& Message() const { return message_; }

    // --- 検証用 ---------------------------------------------------------------
    const std::vector<int>& Choices() const { return choices_; }
    bool ChoosingReplacement() const { return step_ == Step::Replace; }
    const Rect& CardRect(int index) const { return cards_[static_cast<size_t>(index)]; }
    const Rect& SlotRect(int index) const { return slots_[static_cast<size_t>(index)]; }
    const Rect& SkipRect() const { return skipButton_.GetRect(); }

private:
    enum class Step
    {
        Choose,    // 3 つの候補から選ぶ
        Replace    // 入れ替えるスロットを選ぶ
    };

    void Pick(PlayerData& data, int index);
    void Replace(PlayerData& data, int slot);

    std::vector<int> choices_;
    Step  step_ = Step::Choose;
    int   selected_ = 0;
    int   slotSelected_ = 0;
    bool  open_ = false;
    bool  changed_ = false;
    float openTimer_ = 0.0f;
    std::string message_;

    Rect window_;
    std::vector<Rect> cards_;
    std::vector<Rect> slots_;
    Button skipButton_;
    Button backButton_;
};

} // namespace ui
} // namespace ecl
