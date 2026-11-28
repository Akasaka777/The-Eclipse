//==============================================================================
// RogueShopPanel.h : ローグライクモードのショップ（フロアに置かれた行商人）
//   品ぞろえは店ごとにランダム（回復・バフアイテム 4 種・装備 2 つ・強化結晶）。
//   ラン中に拾った col で買う。開いている間はゲームが止まる。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {

class Input;
class PlayerData;

namespace rogue {
struct RunState;
struct ShopStock;
}

namespace ui {

class RogueShopPanel
{
public:
    RogueShopPanel();

    void Open(int floorIndex, int spotIndex);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, rogue::RunState& run, PlayerData& data);
    void Draw(const rogue::RunState& run, const PlayerData& data) const;

    // --- 検証用 ---------------------------------------------------------------
    const Rect& RowRect(int index) const { return rows_[static_cast<size_t>(index)]; }
    const Rect& BuyRect() const { return buyButton_.GetRect(); }
    const Rect& CloseRect() const { return closeButton_.GetRect(); }
    int Selected() const { return selected_; }
    const std::string& Message() const { return message_; }

private:
    const rogue::ShopStock* Stock(const rogue::RunState& run) const;
    void TryBuy(rogue::RunState& run, PlayerData& data);

    bool  open_ = false;
    int   floorIndex_ = 0;
    int   spotIndex_ = 0;
    int   selected_ = 0;
    float openTimer_ = 0.0f;
    std::string message_;
    bool  messageGood_ = true;

    Rect window_;
    std::vector<Rect> rows_;
    Button buyButton_;
    Button closeButton_;
};

} // namespace ui
} // namespace ecl
