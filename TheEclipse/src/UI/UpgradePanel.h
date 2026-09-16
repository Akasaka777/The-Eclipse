//==============================================================================
// UpgradePanel.h : 装備強化タブ
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

class UpgradePanel
{
public:
    UpgradePanel();

    void Open();
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }

private:
    void Layout();
    std::vector<const EquipmentItem*> SortedItems(const GameContext& context) const;

    Rect   window_;
    Button closeButton_;
    Button upgradeButton_;
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
