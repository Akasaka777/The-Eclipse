//==============================================================================
// ShopPanel.h : ショップタブ
//   武器 / 装備 / アイテム / アクセサリー を col で購入する。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/Consumable.h"
#include "Game/GameContext.h"
#include "Game/ItemDatabase.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

//------------------------------------------------------------------------------
// ショップの品揃え
//------------------------------------------------------------------------------
enum class ShopTab
{
    Weapon,     // 武器
    Armor,      // 防具（頭・体・盾・腕・手・足）
    Item,       // アイテム（回復・バフ。素材は売らない）
    Accessory,  // アクセサリー
    Count
};

const char* ShopTabName(ShopTab tab);

class ShopPanel
{
public:
    ShopPanel();

    void Open();
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }
    // このフレームで買い物をしたか（所持品が変わったので装備画面を作り直す）
    bool Purchased() const { return purchased_; }

    // --- レイアウト参照（検証用）---------------------------------------------
    const Rect& WindowRect() const { return window_; }
    const Rect& TabRect(int index) const;
    const Rect& BuyRect() const { return buyButton_.GetRect(); }
    const Rect& BuyTenRect() const { return buyTenButton_.GetRect(); }
    Rect RowRect(int row) const;
    ShopTab CurrentTab() const { return tab_; }

private:
    void Layout();
    // 今のタブに並ぶ品
    std::vector<const ItemTemplate*> Goods() const;
    const ItemTemplate* Selected() const;
    // アイテムタブの品（回復・バフ）
    std::vector<const ConsumableDef*> ItemGoods() const;
    const ConsumableDef* SelectedItem() const;
    // 今のタブに並ぶ品の数
    int GoodsCount() const;
    // アイテムをまとめて買える個数（所持上限で頭打ち）
    int ItemBuyCount(const GameContext& context, int wanted) const;
    void BuyItem(GameContext& context, int wanted);
    void DrawItemList(const GameContext& context) const;
    void DrawItemDetail(const GameContext& context) const;
    void DrawList(const GameContext& context) const;
    void DrawDetail(const GameContext& context) const;

    static constexpr int kTabCount = static_cast<int>(ShopTab::Count);

    Rect   window_;
    Rect   listArea_;
    Rect   detailArea_;
    Button tabButtons_[kTabCount];
    Button buyButton_;
    Button buyTenButton_;   // アイテムを 10 個まとめて買う（アイテムタブのみ）
    Button closeButton_;

    ShopTab tab_ = ShopTab::Weapon;
    int   selectedId_ = 0;
    int   scroll_ = 0;
    bool  open_ = false;
    bool  closeRequested_ = false;
    bool  purchased_ = false;

    std::string message_;
    float messageTimer_ = 0.0f;
};

} // namespace ui
} // namespace ecl
