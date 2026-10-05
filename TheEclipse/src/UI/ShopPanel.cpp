#include "UI/ShopPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {

constexpr int   kVisibleRows = 9;
constexpr float kRowHeight = 66.0f;

// ショップで買った装備の個体値（低めに固定。良い個体値はドロップで狙う）
constexpr int kShopIv = 35;

} // namespace

const char* ShopTabName(ShopTab tab)
{
    switch (tab) {
    case ShopTab::Weapon:    return "武器";
    case ShopTab::Armor:     return "装備";
    case ShopTab::Item:      return "アイテム";
    case ShopTab::Accessory: return "アクセサリー";
    default: return "ショップ";
    }
}

ShopPanel::ShopPanel()
{
    Layout();
}

void ShopPanel::Layout()
{
    window_ = Rect::FromXYWH(120.0f, 100.0f, 1680.0f, 880.0f);

    // --- タブ -----------------------------------------------------------------
    const float tabWidth = 240.0f;
    const float tabGap = 12.0f;
    const float tabLeft = window_.left + 40.0f;
    for (int i = 0; i < kTabCount; ++i) {
        const Rect rect = Rect::FromXYWH(tabLeft + (tabWidth + tabGap) * static_cast<float>(i),
                                         window_.top + 70.0f, tabWidth, 48.0f);
        tabButtons_[i] = Button(rect, ShopTabName(static_cast<ShopTab>(i)), FontSize::Small);
    }

    listArea_ = Rect(window_.left + 40.0f, window_.top + 140.0f, window_.left + 980.0f,
                     window_.top + 140.0f + kRowHeight * static_cast<float>(kVisibleRows));
    detailArea_ = Rect(window_.left + 1010.0f, window_.top + 140.0f, window_.right - 40.0f,
                       window_.bottom - 100.0f);

    buyButton_ = Button(Rect::FromXYWH(detailArea_.left, window_.bottom - 82.0f, 220.0f, 54.0f),
                        "購入する");
    buyButton_.SetAccent(palette::kAccentWarm);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 82.0f,
                                         160.0f, 54.0f), "閉じる");
}

const Rect& ShopPanel::TabRect(int index) const
{
    return tabButtons_[math::ClampInt(index, 0, kTabCount - 1)].GetRect();
}

Rect ShopPanel::RowRect(int row) const
{
    const int clamped = math::ClampInt(row, 0, kVisibleRows - 1);
    return Rect(listArea_.left, listArea_.top + kRowHeight * static_cast<float>(clamped),
                listArea_.right, listArea_.top + kRowHeight * static_cast<float>(clamped + 1) - 6.0f);
}

void ShopPanel::Open()
{
    open_ = true;
    closeRequested_ = false;
    purchased_ = false;
    tab_ = ShopTab::Weapon;
    selectedId_ = 0;
    scroll_ = 0;
    message_.clear();
    messageTimer_ = 0.0f;
}

std::vector<const ItemTemplate*> ShopPanel::Goods() const
{
    const ItemDatabase& db = ItemDatabase::Instance();
    switch (tab_) {
    case ShopTab::Weapon:    return db.ShopItemsForSlot(EquipSlot::WeaponRight);
    case ShopTab::Armor:     return db.ShopArmors();
    case ShopTab::Accessory: return db.ShopItemsForSlot(EquipSlot::Accessory);
    default:                 return std::vector<const ItemTemplate*>();
    }
}

const ItemTemplate* ShopPanel::Selected() const
{
    if (selectedId_ == 0) return nullptr;
    for (const ItemTemplate* t : Goods()) {
        if (t->id == selectedId_) return t;
    }
    return nullptr;
}

void ShopPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;

    closeRequested_ = false;
    purchased_ = false;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    // --- タブ -----------------------------------------------------------------
    for (int i = 0; i < kTabCount; ++i) {
        tabButtons_[i].SetSelected(static_cast<ShopTab>(i) == tab_);
        if (tabButtons_[i].Update(input, dt)) {
            tab_ = static_cast<ShopTab>(i);
            selectedId_ = 0;
            scroll_ = 0;
            message_.clear();
            // 同じクリックで下の一覧まで拾わないよう、タブ切り替えで抜ける
            return;
        }
    }

    Inventory& inventory = context.player.GetInventory();
    const std::vector<const ItemTemplate*> goods = Goods();
    const int maxScroll = math::MaxI(0, static_cast<int>(goods.size()) - kVisibleRows);

    // --- 一覧の選択 / スクロール ----------------------------------------------
    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());
    if (listArea_.Contains(mouseX, mouseY)) {
        scroll_ = math::ClampInt(scroll_ - input.WheelDelta(), 0, maxScroll);
    }
    if (input.MouseClicked(MouseButton::Left)) {
        for (int row = 0; row < kVisibleRows; ++row) {
            const int index = scroll_ + row;
            if (index >= static_cast<int>(goods.size())) break;
            if (!RowRect(row).Contains(mouseX, mouseY)) continue;
            selectedId_ = goods[static_cast<size_t>(index)]->id;
            message_.clear();
            break;
        }
    }
    scroll_ = math::ClampInt(scroll_, 0, maxScroll);

    // --- 購入 -----------------------------------------------------------------
    const ItemTemplate* selected = Selected();
    buyButton_.SetEnabled(selected != nullptr && inventory.Col() >= selected->price);
    if (buyButton_.Update(input, dt) && buyButton_.Enabled() && selected) {
        inventory.AddCol(-selected->price);
        const int iv = (selected->slot == EquipSlot::Accessory) ? kAccessoryIv : kShopIv;
        inventory.AddItem(ItemDatabase::Instance().Create(selected->id, iv));
        purchased_ = true;
        message_ = str::Format("%s を購入しました", selected->name);
        messageTimer_ = 2.4f;
    } else if (selected && inventory.Col() < selected->price
               && input.MouseClicked(MouseButton::Left)
               && buyButton_.GetRect().Contains(mouseX, mouseY)) {
        message_ = "col が足りません";
        messageTimer_ = 2.0f;
    }

    if (closeButton_.Update(input, dt)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void ShopPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "ショップ");

    for (int i = 0; i < kTabCount; ++i) tabButtons_[i].Draw();

    draw::Text(FontSize::Normal, window_.right - 40.0f, window_.top + 78.0f, palette::kAccentWarm,
               str::Format("%s col", str::Comma(context.player.GetInventory().Col()).c_str()),
               draw::TextAlign::Right);

    DrawList(context);
    DrawDetail(context);

    buyButton_.Draw();
    closeButton_.Draw();

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, listArea_.left, window_.bottom - 70.0f, palette::kAccent,
                   message_);
    }
}

void ShopPanel::DrawList(const GameContext& context) const
{
    draw::FillRect(listArea_.Expanded(6.0f), palette::kPanelDark, 190);
    draw::StrokeRect(listArea_.Expanded(6.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    if (tab_ == ShopTab::Item) {
        draw::Text(FontSize::Normal, listArea_.CenterX(), listArea_.CenterY(),
                   palette::kTextDisabled, "アイテムは今後追加予定です", draw::TextAlign::Center);
        return;
    }

    const int col = context.player.GetInventory().Col();
    const std::vector<const ItemTemplate*> goods = Goods();
    const float mouseX = static_cast<float>(Input::Instance().MouseX());
    const float mouseY = static_cast<float>(Input::Instance().MouseY());

    for (int row = 0; row < kVisibleRows; ++row) {
        const int index = scroll_ + row;
        if (index >= static_cast<int>(goods.size())) break;

        const ItemTemplate& t = *goods[static_cast<size_t>(index)];
        const Rect rect = RowRect(row);
        const bool selected = (t.id == selectedId_);
        const bool hovered = rect.Contains(mouseX, mouseY);
        const bool affordable = (col >= t.price);

        const ColorRGB accent = (t.slot == EquipSlot::WeaponRight) ? palette::kAccent
                                                                   : palette::kAccentWarm;
        ColorRGB fill = palette::kPanelDark;
        if (selected) fill = ColorRGB::Lerp(palette::kPanelLight, accent.Scaled(0.5f), 0.55f);
        else if (hovered) fill = palette::kPanelLight;

        draw::GradientRectH(rect, fill, fill.Scaled(0.75f), 235, 12);
        draw::StrokeRect(rect, selected ? accent : palette::kBorder.Scaled(0.7f),
                         selected ? 2.0f : 1.0f, 255);
        draw::FillRect(Rect(rect.left, rect.top, rect.left + 6.0f, rect.bottom), accent, 255);

        const Rect iconRect(rect.left + 12.0f, rect.top + 6.0f, rect.left + 62.0f, rect.bottom - 6.0f);
        if (t.slot == EquipSlot::WeaponRight) DrawWeaponIcon(iconRect, t.weaponType, accent);
        else DrawSlotIcon(iconRect, t.slot, accent);

        draw::Text(FontSize::Normal, rect.left + 74.0f, rect.top + 8.0f, palette::kText, t.name);

        std::string sub;
        if (t.slot == EquipSlot::Accessory) {
            sub = str::Format("%s  %s", AccessoryKindName(t.accessory), t.effect);
        } else if (t.slot == EquipSlot::WeaponRight) {
            sub = str::Format("%s  ATK %d", WeaponTypeName(t.weaponType),
                              static_cast<int>(t.base.attack));
        } else {
            sub = str::Format("%s  DEF %d  HP %d", EquipSlotName(t.slot),
                              static_cast<int>(t.base.defense),
                              static_cast<int>(t.base.maxHp));
        }
        draw::Text(FontSize::Small, rect.left + 74.0f, rect.top + 36.0f, palette::kTextDim, sub);

        draw::Text(FontSize::Small, rect.right - 14.0f, rect.top + 20.0f,
                   affordable ? palette::kAccentWarm : palette::kTextDisabled,
                   str::Format("%s col", str::Comma(t.price).c_str()), draw::TextAlign::Right);
    }

    // スクロールバー
    if (static_cast<int>(goods.size()) > kVisibleRows) {
        const Rect track(listArea_.right + 10.0f, listArea_.top, listArea_.right + 18.0f,
                         listArea_.bottom);
        draw::FillRect(track, palette::kPanelDark, 200);
        const float ratio = static_cast<float>(kVisibleRows) / static_cast<float>(goods.size());
        const float offset = static_cast<float>(scroll_) / static_cast<float>(goods.size());
        draw::FillRect(Rect(track.left, track.top + track.Height() * offset, track.right,
                            track.top + track.Height() * (offset + ratio)),
                       palette::kAccent, 220);
    }

    draw::Text(FontSize::Tiny, listArea_.left, listArea_.bottom + 14.0f, palette::kTextDim,
               str::Format("%s : %d 件（ホイールでスクロール）", ShopTabName(tab_),
                           static_cast<int>(goods.size())));
}

void ShopPanel::DrawDetail(const GameContext& context) const
{
    draw::FillRect(detailArea_, palette::kPanelDark, 205);
    draw::StrokeRect(detailArea_, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    const ItemTemplate* selected = Selected();
    if (!selected) {
        draw::Text(FontSize::Small, detailArea_.CenterX(), detailArea_.top + 24.0f,
                   palette::kTextDisabled,
                   tab_ == ShopTab::Item ? "準備中です" : "品物を選んでください",
                   draw::TextAlign::Center);
        return;
    }

    const float x = detailArea_.left + 24.0f;
    float y = detailArea_.top + 20.0f;

    draw::Text(FontSize::Medium, x, y, palette::kText, selected->name); y += 44.0f;
    draw::Text(FontSize::Tiny, x, y, palette::kTextDim, selected->flavor); y += 32.0f;
    draw::Line(x, y, detailArea_.right - 24.0f, y, palette::kBorder, 1.0f, 120); y += 16.0f;

    if (selected->slot == EquipSlot::Accessory) {
        draw::Text(FontSize::Small, x, y, palette::kAccent,
                   str::Format("種別  %s", AccessoryKindName(selected->accessory))); y += 34.0f;
        draw::Text(FontSize::Small, x, y, palette::kAccentWarm,
                   str::Format("効果  %s", selected->effect)); y += 40.0f;
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   "アクセサリーは 1 つだけ装備できます。"); y += 24.0f;
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   "摩耗も強化もしないので、効果は常に表どおりです。"); y += 34.0f;
    } else {
        // 購入時の個体値での性能を出す（実際に手に入る数値）
        const EquipmentItem sample = ItemDatabase::Instance().Create(selected->id, kShopIv);
        const Stats stats = sample.TotalStats();

        struct Row { const char* label; std::string value; };
        std::vector<Row> rows;
        if (selected->slot == EquipSlot::WeaponRight) {
            rows.push_back({ "武器種",           WeaponTypeName(selected->weaponType) });
            rows.push_back({ "攻撃力",           str::Format("%d", static_cast<int>(stats.attack)) });
            rows.push_back({ "クリティカル率",   str::Format("%.1f%%", stats.critRate * 100.0f) });
            rows.push_back({ "クリティカル倍率", str::Format("+%.0f%%", stats.critDamage * 100.0f) });
            rows.push_back({ "攻撃速度",         str::Format("%+.0f%%", stats.attackSpeed * 100.0f) });
        } else {
            rows.push_back({ "部位",     EquipSlotName(selected->slot) });
            rows.push_back({ "防御力",   str::Format("%d", static_cast<int>(stats.defense)) });
            rows.push_back({ "最大HP",   str::Format("%+d", static_cast<int>(stats.maxHp)) });
            rows.push_back({ "最大MP",   str::Format("%+d", static_cast<int>(stats.maxMp)) });
            rows.push_back({ "移動速度", str::Format("%+d", static_cast<int>(stats.moveSpeed)) });
        }
        for (const Row& row : rows) {
            draw::Text(FontSize::Small, x, y, palette::kTextDim, row.label);
            draw::Text(FontSize::Small, detailArea_.right - 24.0f, y, palette::kText, row.value,
                       draw::TextAlign::Right);
            y += 32.0f;
        }
        y += 8.0f;
        draw::Text(FontSize::Small, x, y, palette::kText,
                   str::Format("戦力 %d", sample.Power())); y += 34.0f;
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   str::Format("個体値 %d で手に入ります（より良い個体値はドロップで狙えます）",
                               kShopIv));
        y += 30.0f;
    }

    draw::Line(x, y, detailArea_.right - 24.0f, y, palette::kBorder, 1.0f, 120); y += 16.0f;

    const int col = context.player.GetInventory().Col();
    const bool affordable = (col >= selected->price);
    draw::Text(FontSize::Normal, x, y, palette::kTextDim, "価格");
    draw::Text(FontSize::Normal, detailArea_.right - 24.0f, y,
               affordable ? palette::kAccentWarm : palette::kDanger,
               str::Format("%s col", str::Comma(selected->price).c_str()),
               draw::TextAlign::Right);
    y += 34.0f;
    if (!affordable) {
        draw::Text(FontSize::Tiny, x, y, palette::kDanger,
                   str::Format("あと %s col 必要です",
                               str::Comma(selected->price - col).c_str()));
    }
}

} // namespace ui
} // namespace ecl
