//==============================================================================
// SmithPanel.h : 鍛冶屋タブ
//   装備の強化と、耐久力の修理を行う。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

class SmithPanel
{
public:
    SmithPanel();

    void Open();
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }

private:
    void Layout();
    std::vector<const EquipmentItem*> SortedItems(const GameContext& context) const;
    // 一括修理の費用などをまとめて表示する
    void DrawRepairSummary(const GameContext& context) const;

    Rect   window_;
    Button closeButton_;
    Button upgradeButton_;
    Button repairButton_;
    Button repairAllButton_;
    Button filterButton_;

    int   selectedUid_ = 0;
    int   scroll_ = 0;
    bool  equippedOnly_ = false;
    bool  open_ = false;
    bool  closeRequested_ = false;

    // 演出
    float resultTimer_ = 0.0f;
    bool  lastSuccess_ = false;
    std::string message_;
};

} // namespace ui
} // namespace ecl
