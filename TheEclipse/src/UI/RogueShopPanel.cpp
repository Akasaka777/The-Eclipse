#include "UI/RogueShopPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Game/Consumable.h"
#include "Game/PlayerData.h"
#include "Game/Roguelike.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {
constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
constexpr int   kMaxRows = 8;
constexpr float kRowHeight = 74.0f;
constexpr float kInputDelay = 0.25f;
} // namespace

RogueShopPanel::RogueShopPanel()
{
    window_ = Rect::FromCenter(kScreenW * 0.5f, kScreenH * 0.5f, 1100.0f, 860.0f);
    for (int i = 0; i < kMaxRows; ++i) {
        rows_.push_back(Rect::FromXYWH(window_.left + 40.0f, window_.top + 140.0f + kRowHeight * static_cast<float>(i),
                                       window_.Width() - 80.0f, kRowHeight - 8.0f));
    }
    buyButton_ = Button(Rect::FromXYWH(window_.right - 460.0f, window_.bottom - 86.0f, 200.0f, 58.0f), "買う");
    buyButton_.SetAccent(palette::kAccentWarm);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 240.0f, window_.bottom - 86.0f, 200.0f, 58.0f), "閉じる");
}

void RogueShopPanel::Open(int floorIndex, int spotIndex)
{
    open_ = true;
    floorIndex_ = floorIndex;
    spotIndex_ = spotIndex;
    selected_ = 0;
    openTimer_ = 0.0f;
    message_.clear();
}

const rogue::ShopStock* RogueShopPanel::Stock(const rogue::RunState& run) const
{
    for (const rogue::ShopStock& shop : run.shops) {
        if (shop.floorIndex == floorIndex_ && shop.spotIndex == spotIndex_) return &shop;
    }
    return nullptr;
}

void RogueShopPanel::TryBuy(rogue::RunState& run, PlayerData& data)
{
    rogue::ShopStock& shop = rogue::ShopAt(run, floorIndex_, spotIndex_);
    if (selected_ < 0 || selected_ >= static_cast<int>(shop.offers.size())) return;
    const rogue::ShopOffer& offer = shop.offers[static_cast<size_t>(selected_)];
    std::string name;
    if (offer.kind == rogue::OfferKind::Consumable) {
        const ConsumableDef* def = ConsumableDatabase::Instance().Find(offer.itemId);
        name = def ? def->name : "アイテム";
    } else if (offer.kind == rogue::OfferKind::Equipment) {
        name = offer.item.DisplayName();
    } else {
        name = str::Format("強化結晶 ×%d", offer.amount);
    }

    if (offer.stock <= 0) {
        message_ = "売り切れです";
        messageGood_ = false;
    } else if (data.GetInventory().Col() < offer.price) {
        message_ = "col が足りません";
        messageGood_ = false;
    } else if (rogue::Buy(data, shop, selected_)) {
        message_ = (offer.kind == rogue::OfferKind::Equipment)
            ? str::Format("%s を買った（ESC のメニュー →「装備」で付け替え）", name.c_str())
            : str::Format("%s を買った", name.c_str());
        messageGood_ = true;
    } else {
        message_ = "これ以上持てません";
        messageGood_ = false;
    }
}

void RogueShopPanel::Update(float dt, const Input& input, rogue::RunState& run, PlayerData& data)
{
    if (!open_) return;
    openTimer_ += dt;
    const bool ready = openTimer_ >= kInputDelay;
    rogue::ShopStock& shop = rogue::ShopAt(run, floorIndex_, spotIndex_);
    const int count = math::MinI(kMaxRows, static_cast<int>(shop.offers.size()));
    if (count <= 0) return;

    if (input.Pressed(GameAction::MoveUp)) selected_ = (selected_ + count - 1) % count;
    if (input.Pressed(GameAction::MoveDown)) selected_ = (selected_ + 1) % count;

    const float mx = static_cast<float>(input.MouseX());
    const float my = static_cast<float>(input.MouseY());
    if (ready && input.MouseClicked(MouseButton::Left)) {
        for (int i = 0; i < count; ++i) {
            if (!rows_[static_cast<size_t>(i)].Contains(mx, my)) continue;
            // 選んでいる行をもう一度クリックすると買う
            if (i == selected_) TryBuy(run, data);
            selected_ = i;
        }
    }
    if (buyButton_.Update(input, dt) && ready) TryBuy(run, data);
    if (ready && input.Pressed(GameAction::Confirm)) TryBuy(run, data);
    if (closeButton_.Update(input, dt) || (ready && (input.Pressed(GameAction::Cancel) || input.Pressed(GameAction::Menu)))) {
        open_ = false;
    }
}

void RogueShopPanel::Draw(const rogue::RunState& run, const PlayerData& data) const
{
    if (!open_) return;
    DrawDimOverlay(160);
    DrawWindow(window_, "行商人のショップ");

    const Inventory& inventory = data.GetInventory();
    draw::Text(FontSize::Normal, window_.right - 40.0f, window_.top + 84.0f, palette::kAccentWarm,
               str::Format("所持 %s col ／ 強化結晶 %d", str::Comma(inventory.Col()).c_str(), inventory.Material()),
               draw::TextAlign::Right);
    draw::Text(FontSize::Small, window_.left + 40.0f, window_.top + 88.0f, palette::kTextDim,
               "ラン中に拾った col で買えます（本編の col は使えません）");

    const rogue::ShopStock* shop = Stock(run);
    if (!shop) return;
    for (size_t i = 0; i < shop->offers.size() && i < rows_.size(); ++i) {
        const rogue::ShopOffer& offer = shop->offers[i];
        const Rect& row = rows_[i];
        const bool selected = (static_cast<int>(i) == selected_);
        const bool soldOut = offer.stock <= 0;
        const bool affordable = inventory.Col() >= offer.price;

        draw::GradientRectH(row, selected ? palette::kPanelLight : palette::kPanelDark, palette::kPanelDark, 235, 10);
        draw::StrokeRect(row, selected ? palette::kAccent : palette::kBorder.Scaled(0.6f), selected ? 2.0f : 1.0f, 255);

        const Rect icon(row.left + 10.0f, row.top + 6.0f, row.left + 62.0f, row.bottom - 6.0f);
        std::string name;
        std::string detail;
        switch (offer.kind) {
        case rogue::OfferKind::Consumable:
            if (const ConsumableDef* def = ConsumableDatabase::Instance().Find(offer.itemId)) {
                DrawConsumableIcon(icon, *def, soldOut);
                name = def->name;
                detail = str::Format("%s ／ 所持 %d", def->effect.c_str(), inventory.ItemCount(def->id));
            }
            break;
        case rogue::OfferKind::Equipment:
            if (offer.item.IsWeapon()) DrawWeaponIcon(icon, offer.item.weaponType, palette::kAccent);
            else DrawSlotIcon(icon, offer.item.slot, palette::kAccentWarm);
            name = offer.item.DisplayName();
            detail = offer.item.IsWeapon()
                ? str::Format("%s ATK %d ／ 戦力 %d", WeaponTypeName(offer.item.weaponType),
                              static_cast<int>(offer.item.TotalStats().attack), offer.item.Power())
                : str::Format("%s DEF %d HP %d ／ 戦力 %d", EquipSlotName(offer.item.slot),
                              static_cast<int>(offer.item.TotalStats().defense),
                              static_cast<int>(offer.item.TotalStats().maxHp), offer.item.Power());
            break;
        case rogue::OfferKind::Crystal:
            draw::Glow(icon.CenterX(), icon.CenterY(), 26.0f, ColorRGB(120, 210, 255), 90, 3);
            draw::Triangle(Vec2(icon.CenterX(), icon.top + 4.0f), Vec2(icon.left + 12.0f, icon.CenterY()),
                           Vec2(icon.right - 12.0f, icon.CenterY()), ColorRGB(120, 210, 255), true, 255);
            draw::Triangle(Vec2(icon.CenterX(), icon.bottom - 4.0f), Vec2(icon.left + 12.0f, icon.CenterY()),
                           Vec2(icon.right - 12.0f, icon.CenterY()), ColorRGB(80, 150, 200), true, 255);
            name = str::Format("強化結晶 ×%d", offer.amount);
            detail = "鍛冶屋での強化に使う";
            break;
        }
        draw::Text(FontSize::Normal, row.left + 76.0f, row.top + 6.0f, soldOut ? palette::kTextDisabled : palette::kText, name);
        draw::Text(FontSize::Tiny, row.left + 78.0f, row.top + 40.0f, palette::kTextDim, detail);
        draw::Text(FontSize::Normal, row.right - 20.0f, row.top + 6.0f,
                   soldOut ? palette::kTextDisabled : (affordable ? palette::kAccentWarm : palette::kDanger),
                   soldOut ? std::string("売り切れ") : str::Format("%s col", str::Comma(offer.price).c_str()),
                   draw::TextAlign::Right);
        if (!soldOut) {
            draw::Text(FontSize::Tiny, row.right - 20.0f, row.top + 42.0f, palette::kTextDim,
                       str::Format("残り %d", offer.stock), draw::TextAlign::Right);
        }
    }

    if (!message_.empty()) {
        draw::Text(FontSize::Small, window_.left + 40.0f, window_.bottom - 72.0f,
                   messageGood_ ? palette::kHp : palette::kDanger, message_);
    }
    draw::Text(FontSize::Tiny, window_.left + 40.0f, window_.bottom - 34.0f, palette::kTextDisabled,
               "↑↓ で選ぶ ／ ENTER・もう一度クリックで買う ／ ESC で閉じる");
    buyButton_.Draw();
    closeButton_.Draw();
}

} // namespace ui
} // namespace ecl
