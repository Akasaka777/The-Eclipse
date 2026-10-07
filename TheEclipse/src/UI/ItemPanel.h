//==============================================================================
// ItemPanel.h : プレイヤータブの「アイテム」
//   回復 / バフ / 素材 を種類ごとに一覧表示する。
//   回復アイテムとバフアイテムは 1 つずつ「戦闘で使うアイテム」に装備でき、
//   戦闘画面右下のアイテムスライダーから使う。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/Consumable.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

class ItemPanel
{
public:
    ItemPanel();

    void Open();
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }

    // --- 検証用 ---------------------------------------------------------------
    const Rect& WindowRect() const { return window_; }
    const Rect& TabRect(int index) const;
    Rect RowRect(int row) const;
    const Rect& EquipRect() const { return equipButton_.GetRect(); }
    const Rect& UnequipRect() const { return unequipButton_.GetRect(); }
    const Rect& SlotBoxRect(QuickSlot slot) const;
    ConsumableKind CurrentKind() const { return kind_; }
    int SelectedId() const { return selectedId_; }

private:
    void Layout();
    std::vector<int> ListIds(const GameContext& context) const;
    void DrawList(const GameContext& context) const;
    void DrawQuickSlots(const GameContext& context) const;
    void DrawDetail(const GameContext& context) const;

    static constexpr int kKindCount = static_cast<int>(ConsumableKind::Count);

    Rect   window_;
    Rect   listArea_;
    Rect   detailArea_;
    Rect   slotBoxes_[kQuickSlotCount];
    Button tabButtons_[kKindCount];
    Button equipButton_;
    Button unequipButton_;
    Button closeButton_;

    ConsumableKind kind_ = ConsumableKind::Recovery;
    int   selectedId_ = 0;
    int   scroll_ = 0;
    bool  open_ = false;
    bool  closeRequested_ = false;

    std::string message_;
    float messageTimer_ = 0.0f;
};

} // namespace ui
} // namespace ecl
