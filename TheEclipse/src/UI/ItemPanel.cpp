#include "UI/ItemPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {

constexpr int   kVisibleRows = 9;
constexpr float kRowHeight = 66.0f;

// 装備枠の種類 → 装備先（素材は装備できない）
bool QuickSlotFor(ConsumableKind kind, QuickSlot* out)
{
    if (kind == ConsumableKind::Recovery) { *out = QuickSlot::Recovery; return true; }
    if (kind == ConsumableKind::Buff)     { *out = QuickSlot::Buff;     return true; }
    return false;
}

} // namespace

ItemPanel::ItemPanel()
{
    Layout();
}

void ItemPanel::Layout()
{
    window_ = Rect::FromXYWH(120.0f, 100.0f, 1680.0f, 880.0f);

    const float tabWidth = 220.0f;
    const float tabGap = 12.0f;
    for (int i = 0; i < kKindCount; ++i) {
        const Rect rect = Rect::FromXYWH(window_.left + 40.0f + (tabWidth + tabGap) * static_cast<float>(i),
                                         window_.top + 70.0f, tabWidth, 48.0f);
        tabButtons_[i] = Button(rect, ConsumableKindName(static_cast<ConsumableKind>(i)),
                                FontSize::Small);
    }

    listArea_ = Rect(window_.left + 40.0f, window_.top + 140.0f, window_.left + 900.0f,
                     window_.top + 140.0f + kRowHeight * static_cast<float>(kVisibleRows));
    detailArea_ = Rect(window_.left + 930.0f, window_.top + 140.0f, window_.right - 40.0f,
                       window_.bottom - 100.0f);

    // 戦闘で使うアイテム（回復 / バフ）の枠を詳細欄の上部に並べる
    const float boxGap = 16.0f;
    const float boxWidth = (detailArea_.Width() - 48.0f - boxGap) * 0.5f;
    for (int i = 0; i < kQuickSlotCount; ++i) {
        const float left = detailArea_.left + 24.0f + (boxWidth + boxGap) * static_cast<float>(i);
        slotBoxes_[i] = Rect(left, detailArea_.top + 50.0f, left + boxWidth, detailArea_.top + 160.0f);
    }

    equipButton_ = Button(Rect::FromXYWH(detailArea_.left, window_.bottom - 82.0f, 220.0f, 54.0f),
                          "装備する");
    equipButton_.SetAccent(palette::kAccentWarm);
    unequipButton_ = Button(Rect::FromXYWH(detailArea_.left + 236.0f, window_.bottom - 82.0f,
                                           220.0f, 54.0f), "外す");
    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 82.0f,
                                         160.0f, 54.0f), "閉じる");
}

const Rect& ItemPanel::TabRect(int index) const
{
    return tabButtons_[math::ClampInt(index, 0, kKindCount - 1)].GetRect();
}

const Rect& ItemPanel::SlotBoxRect(QuickSlot slot) const
{
    return slotBoxes_[math::ClampInt(static_cast<int>(slot), 0, kQuickSlotCount - 1)];
}

Rect ItemPanel::RowRect(int row) const
{
    const int clamped = math::ClampInt(row, 0, kVisibleRows - 1);
    return Rect(listArea_.left, listArea_.top + kRowHeight * static_cast<float>(clamped),
                listArea_.right, listArea_.top + kRowHeight * static_cast<float>(clamped + 1) - 6.0f);
}

void ItemPanel::Open()
{
    open_ = true;
    closeRequested_ = false;
    kind_ = ConsumableKind::Recovery;
    selectedId_ = 0;
    scroll_ = 0;
    message_.clear();
    messageTimer_ = 0.0f;
}

std::vector<int> ItemPanel::ListIds(const GameContext& context) const
{
    return context.player.GetInventory().OwnedItemIds(kind_);
}

void ItemPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;

    closeRequested_ = false;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    // --- 種類のタブ -------------------------------------------------------------
    for (int i = 0; i < kKindCount; ++i) {
        tabButtons_[i].SetSelected(static_cast<ConsumableKind>(i) == kind_);
        if (tabButtons_[i].Update(input, dt)) {
            kind_ = static_cast<ConsumableKind>(i);
            selectedId_ = 0;
            scroll_ = 0;
            message_.clear();
            return;   // 同じクリックで一覧まで拾わない
        }
    }

    Inventory& inventory = context.player.GetInventory();
    const std::vector<int> ids = ListIds(context);
    const int maxScroll = math::MaxI(0, static_cast<int>(ids.size()) - kVisibleRows);

    // --- 一覧の選択 / スクロール ------------------------------------------------
    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());
    if (listArea_.Contains(mouseX, mouseY)) {
        scroll_ = math::ClampInt(scroll_ - input.WheelDelta(), 0, maxScroll);
    }
    if (input.MouseClicked(MouseButton::Left)) {
        for (int row = 0; row < kVisibleRows; ++row) {
            const int index = scroll_ + row;
            if (index >= static_cast<int>(ids.size())) break;
            if (!RowRect(row).Contains(mouseX, mouseY)) continue;
            selectedId_ = ids[static_cast<size_t>(index)];
            message_.clear();
            break;
        }
        // 装備枠をクリックすると、その枠のアイテムを選ぶ
        for (int i = 0; i < kQuickSlotCount; ++i) {
            if (!slotBoxes_[i].Contains(mouseX, mouseY)) continue;
            const QuickSlot slot = static_cast<QuickSlot>(i);
            kind_ = QuickSlotKind(slot);
            selectedId_ = inventory.QuickItem(slot);
            scroll_ = 0;
            message_.clear();
        }
    }
    scroll_ = math::ClampInt(scroll_, 0, math::MaxI(0, static_cast<int>(ListIds(context).size())
                                                           - kVisibleRows));

    // --- 装備 / 外す --------------------------------------------------------------
    const ConsumableDef* selected = ConsumableDatabase::Instance().Find(selectedId_);
    QuickSlot slot = QuickSlot::Recovery;
    const bool equippable = selected && QuickSlotFor(selected->kind, &slot)
                         && inventory.ItemCount(selected->id) > 0;
    const bool equipped = selected && equippable && inventory.QuickItem(slot) == selected->id;

    equipButton_.SetEnabled(equippable && !equipped);
    equipButton_.SetLabel(equipped ? "装備中" : "装備する");
    unequipButton_.SetEnabled(equipped);

    if (equipButton_.Update(input, dt) && equipButton_.Enabled()) {
        inventory.SetQuickItem(slot, selected->id);
        message_ = str::Format("%s を%sアイテムに装備しました", selected->name.c_str(),
                               QuickSlotName(slot));
        messageTimer_ = 2.4f;
    }
    if (unequipButton_.Update(input, dt) && unequipButton_.Enabled()) {
        inventory.SetQuickItem(slot, 0);
        message_ = str::Format("%s を外しました", selected->name.c_str());
        messageTimer_ = 2.4f;
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void ItemPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "アイテム");
    for (int i = 0; i < kKindCount; ++i) tabButtons_[i].Draw();

    DrawList(context);
    DrawQuickSlots(context);
    DrawDetail(context);

    equipButton_.Draw();
    unequipButton_.Draw();
    closeButton_.Draw();

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, listArea_.left, window_.bottom - 70.0f, palette::kAccent, message_);
    }
}

void ItemPanel::DrawList(const GameContext& context) const
{
    draw::FillRect(listArea_.Expanded(6.0f), palette::kPanelDark, 190);
    draw::StrokeRect(listArea_.Expanded(6.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    const Inventory& inventory = context.player.GetInventory();
    const std::vector<int> ids = ListIds(context);

    if (ids.empty()) {
        draw::Text(FontSize::Normal, listArea_.CenterX(), listArea_.CenterY() - 30.0f,
                   palette::kTextDisabled, "所持していません", draw::TextAlign::Center);
        draw::Text(FontSize::Small, listArea_.CenterX(), listArea_.CenterY() + 10.0f,
                   palette::kTextDim,
                   (kind_ == ConsumableKind::Material) ? "素材はクエストの敵やボスが落とします"
                                                       : "ホームの「ショップ」→「アイテム」で買えます",
                   draw::TextAlign::Center);
    }

    const float mouseX = static_cast<float>(Input::Instance().MouseX());
    const float mouseY = static_cast<float>(Input::Instance().MouseY());

    for (int row = 0; row < kVisibleRows; ++row) {
        const int index = scroll_ + row;
        if (index >= static_cast<int>(ids.size())) break;

        const ConsumableDef* def = ConsumableDatabase::Instance().Find(ids[static_cast<size_t>(index)]);
        if (!def) continue;

        const Rect rect = RowRect(row);
        const bool selected = (def->id == selectedId_);
        const bool hovered = rect.Contains(mouseX, mouseY);
        QuickSlot slot = QuickSlot::Recovery;
        const bool equipped = QuickSlotFor(def->kind, &slot) && inventory.QuickItem(slot) == def->id;

        ColorRGB fill = palette::kPanelDark;
        if (selected) fill = ColorRGB::Lerp(palette::kPanelLight, def->color.Scaled(0.5f), 0.55f);
        else if (hovered) fill = palette::kPanelLight;

        draw::GradientRectH(rect, fill, fill.Scaled(0.75f), 235, 12);
        draw::StrokeRect(rect, selected ? def->color : palette::kBorder.Scaled(0.7f),
                         selected ? 2.0f : 1.0f, 255);
        draw::FillRect(Rect(rect.left, rect.top, rect.left + 6.0f, rect.bottom), def->color, 255);

        DrawConsumableIcon(Rect(rect.left + 12.0f, rect.top + 6.0f, rect.left + 62.0f, rect.bottom - 6.0f),
                           *def);
        draw::Text(FontSize::Normal, rect.left + 74.0f, rect.top + 8.0f, palette::kText, def->name);
        draw::Text(FontSize::Small, rect.left + 74.0f, rect.top + 36.0f, palette::kTextDim, def->effect);

        draw::Text(FontSize::Normal, rect.right - 14.0f, rect.top + 16.0f, palette::kAccentWarm,
                   str::Format("×%d", inventory.ItemCount(def->id)), draw::TextAlign::Right);
        if (equipped) {
            const Rect badge = Rect::FromXYWH(rect.right - 190.0f, rect.top + 16.0f, 92.0f, 28.0f);
            draw::FillRect(badge, palette::kAccent.Scaled(0.35f), 230);
            draw::StrokeRect(badge, palette::kAccent, 1.0f, 230);
            draw::Text(FontSize::Tiny, badge.CenterX(), badge.top + 6.0f, palette::kText, "装備中",
                       draw::TextAlign::Center);
        }
    }

    // スクロールバー
    if (static_cast<int>(ids.size()) > kVisibleRows) {
        const Rect track(listArea_.right + 10.0f, listArea_.top, listArea_.right + 18.0f,
                         listArea_.bottom);
        draw::FillRect(track, palette::kPanelDark, 200);
        const float ratio = static_cast<float>(kVisibleRows) / static_cast<float>(ids.size());
        const float offset = static_cast<float>(scroll_) / static_cast<float>(ids.size());
        draw::FillRect(Rect(track.left, track.top + track.Height() * offset, track.right,
                            track.top + track.Height() * (offset + ratio)),
                       palette::kAccent, 220);
    }

    draw::Text(FontSize::Tiny, listArea_.left, listArea_.bottom + 14.0f, palette::kTextDim,
               str::Format("%s : %d 種類（ホイールでスクロール）", ConsumableKindName(kind_),
                           static_cast<int>(ids.size())));
}

void ItemPanel::DrawQuickSlots(const GameContext& context) const
{
    draw::FillRect(detailArea_, palette::kPanelDark, 205);
    draw::StrokeRect(detailArea_, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    draw::Text(FontSize::Small, detailArea_.left + 24.0f, detailArea_.top + 16.0f, palette::kAccent,
               "戦闘で使うアイテム");

    const Inventory& inventory = context.player.GetInventory();
    for (int i = 0; i < kQuickSlotCount; ++i) {
        const QuickSlot slot = static_cast<QuickSlot>(i);
        const Rect& box = slotBoxes_[i];
        const ConsumableDef* def = ConsumableDatabase::Instance().Find(inventory.QuickItem(slot));
        const bool chosen = def && def->id == selectedId_;

        draw::FillRect(box, palette::kPanel, 220);
        draw::StrokeRect(box, chosen ? def->color : palette::kBorder.Scaled(0.7f),
                         chosen ? 2.0f : 1.0f, 230);
        draw::Text(FontSize::Tiny, box.left + 10.0f, box.top + 6.0f, palette::kTextDim,
                   str::Format("%sアイテム", QuickSlotName(slot)));

        const Rect icon = Rect::FromXYWH(box.left + 10.0f, box.top + 32.0f, 64.0f, 64.0f);
        if (!def) {
            draw::StrokeRect(icon, palette::kTextDisabled, 1.0f, 150);
            draw::Text(FontSize::Small, icon.right + 14.0f, box.top + 50.0f, palette::kTextDisabled,
                       "未装備");
            continue;
        }
        const int count = inventory.ItemCount(def->id);
        DrawConsumableIcon(icon, *def, count <= 0);
        draw::Text(FontSize::Small, icon.right + 14.0f, box.top + 36.0f,
                   count > 0 ? palette::kText : palette::kTextDisabled, def->name);
        draw::Text(FontSize::Small, icon.right + 14.0f, box.top + 66.0f,
                   count > 0 ? palette::kAccentWarm : palette::kDanger,
                   str::Format("×%d", count));
    }
}

void ItemPanel::DrawDetail(const GameContext& context) const
{
    const float x = detailArea_.left + 24.0f;
    const float right = detailArea_.right - 24.0f;
    float y = slotBoxes_[0].bottom + 20.0f;
    draw::Line(x, y, right, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;

    const ConsumableDef* selected = ConsumableDatabase::Instance().Find(selectedId_);
    if (!selected) {
        draw::Text(FontSize::Small, detailArea_.CenterX(), y + 10.0f, palette::kTextDisabled,
                   "アイテムを選んでください", draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, detailArea_.CenterX(), y + 48.0f, palette::kTextDim,
                   "回復とバフを 1 つずつ装備すると、戦闘画面の右下から使えます",
                   draw::TextAlign::Center);
        return;
    }

    const Inventory& inventory = context.player.GetInventory();
    const int count = inventory.ItemCount(selected->id);

    DrawConsumableIcon(Rect::FromXYWH(x, y, 72.0f, 72.0f), *selected, count <= 0);
    draw::Text(FontSize::Medium, x + 88.0f, y + 2.0f, palette::kText, selected->name);
    draw::Text(FontSize::Small, x + 88.0f, y + 42.0f, selected->color,
               ConsumableKindName(selected->kind));
    y += 92.0f;

    draw::Text(FontSize::Tiny, x, y, palette::kTextDim, selected->description);
    y += 34.0f;

    draw::Text(FontSize::Small, x, y, palette::kTextDim, "効果");
    draw::Text(FontSize::Small, right, y, palette::kAccentWarm, selected->effect, draw::TextAlign::Right);
    y += 36.0f;
    draw::Text(FontSize::Small, x, y, palette::kTextDim, "所持数");
    draw::Text(FontSize::Small, right, y, palette::kText,
               str::Format("%d / %d", count, selected->MaxStack()), draw::TextAlign::Right);
    y += 44.0f;

    draw::Line(x, y, right, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;

    switch (selected->kind) {
    case ConsumableKind::Recovery:
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   "戦闘中に右下のスライダーで「回復」を選び、E キーかクリックで使います。");
        y += 24.0f;
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   "HP / MP が満タンのときは使われません。");
        break;
    case ConsumableKind::Buff:
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   "戦闘中に右下のスライダーで「バフ」を選び、E キーかクリックで使います。");
        y += 24.0f;
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   "使ったアイテムは無くなります。別のバフを使うと効果が上書きされます。");
        break;
    default:
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim,
                   "武器・防具の作成に使う素材です（作成は今後実装予定）。");
        y += 24.0f;
        draw::Text(FontSize::Tiny, x, y, palette::kTextDim, "装備や使用はできません。");
        break;
    }
}

} // namespace ui
} // namespace ecl
