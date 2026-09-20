//==============================================================================
// SmithPanel.h : 鍛冶屋タブ
//   装備の強化と、耐久力の修理、そして売却を行う。
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
    // 選択中の装備の売却額を案内する
    void DrawSellInfo(const GameContext& context) const;
    // 強化の伸びを 1 行描く（結果が無ければ「???」）
    void DrawGrowthLine(float x, float y, const char* label, float before, float after,
                        bool hasResult, bool percent) const;

    Rect   window_;
    Button closeButton_;
    Button upgradeButton_;
    Button repairButton_;
    Button repairAllButton_;
    Button sellButton_;
    Button filterButton_;
    // 装備の種類で絞り込むタブ
    std::vector<Button> categoryButtons_;
    // 使う強化結晶の個数を選ぶ
    Button crystalMinusButton_;
    Button crystalPlusButton_;

    int   selectedUid_ = 0;
    int   scroll_ = 0;
    int   categoryIndex_ = 0;   // 0 = すべて、以降は EquipSlot に対応
    int   crystals_ = 1;        // 使う強化結晶の個数
    int   confirmSellUid_ = 0;  // 売却の確認待ち（誤操作で強化品を失わないように 2 度押し）
    bool  equippedOnly_ = false;
    bool  open_ = false;
    bool  closeRequested_ = false;

    // 演出
    float resultTimer_ = 0.0f;
    bool  lastSuccess_ = false;
    std::string message_;

    // 直前の強化結果（同じ装備を選んでいる間だけ表示する）
    int   lastResultUid_ = 0;
    Stats lastBefore_;
    Stats lastAfter_;
};

} // namespace ui
} // namespace ecl
