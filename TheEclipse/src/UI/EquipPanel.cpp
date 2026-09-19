#include "UI/EquipPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/CharacterArt.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {
constexpr int   kVisibleRows = 8;
constexpr float kRowHeight = 66.0f;
} // namespace

EquipPanel::EquipPanel()
{
    Layout();
}

void EquipPanel::Layout()
{
    window_ = Rect::FromXYWH(120.0f, 100.0f, 1680.0f, 880.0f);

    slotButtons_.clear();
    float y = window_.top + 88.0f;
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        slotButtons_.push_back(Button(Rect::FromXYWH(window_.left + 36.0f, y, 300.0f, 66.0f), ""));
        y += 74.0f;
    }

    const float listLeft = window_.left + 372.0f;
    equipButton_ = Button(Rect::FromXYWH(listLeft, window_.bottom - 82.0f, 180.0f, 54.0f), "装備する");
    unequipButton_ = Button(Rect::FromXYWH(listLeft + 196.0f, window_.bottom - 82.0f, 180.0f, 54.0f), "外す");
    sellButton_ = Button(Rect::FromXYWH(listLeft + 392.0f, window_.bottom - 82.0f, 180.0f, 54.0f), "売却");
    sellButton_.SetAccent(palette::kAccentWarm);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 82.0f, 160.0f, 54.0f),
                          "閉じる");
}

void EquipPanel::Open()
{
    open_ = true;
    closeRequested_ = false;
    equipmentChanged_ = false;
    selectedUid_ = 0;
    scroll_ = 0;
    message_.clear();
    messageTimer_ = 0.0f;
}

void EquipPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;
    equipmentChanged_ = false;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    Inventory& inventory = context.player.GetInventory();

    // --- スロット選択 --------------------------------------------------------
    for (int i = 0; i < static_cast<int>(slotButtons_.size()); ++i) {
        const EquipSlot slot = static_cast<EquipSlot>(i);
        slotButtons_[static_cast<size_t>(i)].SetSelected(slot == selectedSlot_);
        if (slotButtons_[static_cast<size_t>(i)].Update(input, dt)) {
            selectedSlot_ = slot;
            selectedUid_ = 0;
            scroll_ = 0;
        }
    }

    // --- 一覧の選択 / スクロール ----------------------------------------------
    const std::vector<const EquipmentItem*> items = inventory.ItemsForSlot(selectedSlot_);
    const int maxScroll = math::MaxI(0, static_cast<int>(items.size()) - kVisibleRows);

    const float listLeft = window_.left + 372.0f;
    const Rect listArea(listLeft, window_.top + 90.0f, listLeft + 700.0f,
                        window_.top + 90.0f + kRowHeight * static_cast<float>(kVisibleRows));

    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());

    if (listArea.Contains(mouseX, mouseY)) {
        const int wheel = input.WheelDelta();
        if (wheel != 0) scroll_ = math::ClampInt(scroll_ - wheel, 0, maxScroll);

        if (input.MouseClicked(MouseButton::Left)) {
            const int index = scroll_ + static_cast<int>((mouseY - listArea.top) / kRowHeight);
            if (index >= 0 && index < static_cast<int>(items.size())) {
                selectedUid_ = items[static_cast<size_t>(index)]->uid;
            }
        }
    }
    scroll_ = math::ClampInt(scroll_, 0, maxScroll);

    // --- ボタン -------------------------------------------------------------
    const EquipmentItem* selected = inventory.FindByUid(selectedUid_);
    const bool canEquipHere = selected != nullptr && inventory.CanEquipTo(selectedUid_, selectedSlot_)
                           && inventory.EquippedUid(selectedSlot_) != selectedUid_;
    equipButton_.SetEnabled(canEquipHere);
    unequipButton_.SetEnabled(inventory.EquippedUid(selectedSlot_) != 0);
    sellButton_.SetEnabled(sellEnabled_ && selected != nullptr
                           && !inventory.IsEquipped(selectedUid_));

    if (equipButton_.Update(input, dt) && equipButton_.Enabled()) {
        if (inventory.EquipTo(selectedUid_, selectedSlot_)) {
            // 武器が変わったらスキルスロットを組み直す
            context.player.RefreshSkillLoadoutForEquipment();
            equipmentChanged_ = true;
            message_ = "装備を変更しました";
            messageTimer_ = 2.0f;
        }
    }
    if (unequipButton_.Update(input, dt) && unequipButton_.Enabled()) {
        inventory.Unequip(selectedSlot_);
        context.player.RefreshSkillLoadoutForEquipment();
        equipmentChanged_ = true;
        message_ = "装備を外しました";
        messageTimer_ = 2.0f;
    }
    if (sellEnabled_ && sellButton_.Update(input, dt) && sellButton_.Enabled()) {
        const int value = inventory.SellValue(selectedUid_);
        if (inventory.Sell(selectedUid_)) {
            message_ = str::Format("売却しました（+%s col）", str::Comma(value).c_str());
            messageTimer_ = 2.0f;
            selectedUid_ = 0;
        }
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void EquipPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "装備");
    DrawSlotColumn(context);
    DrawItemList(context);
    DrawComparison(context);

    equipButton_.Draw();
    unequipButton_.Draw();
    if (sellEnabled_) sellButton_.Draw();
    closeButton_.Draw();

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, window_.left + 372.0f, window_.bottom - 112.0f, palette::kAccent,
                   message_);
    }
}

void EquipPanel::DrawSlotColumn(const GameContext& context) const
{
    const Inventory& inventory = context.player.GetInventory();

    for (int i = 0; i < static_cast<int>(slotButtons_.size()); ++i) {
        const EquipSlot slot = static_cast<EquipSlot>(i);
        const Rect rect = slotButtons_[static_cast<size_t>(i)].GetRect();
        const bool selected = (slot == selectedSlot_);
        const EquipmentItem* equipped = inventory.Equipped(slot);

        const ColorRGB base = selected ? palette::kPanelLight : palette::kPanelDark;
        draw::GradientRectH(rect, base, base.Scaled(0.7f), 235, 10);
        draw::StrokeRect(rect, selected ? palette::kAccent : palette::kBorder.Scaled(0.6f),
                         selected ? 3.0f : 1.0f, 255);

        const ColorRGB iconColor = equipped ? palette::kAccent : palette::kTextDisabled;
        DrawSlotIcon(Rect(rect.left + 6.0f, rect.top + 6.0f, rect.left + 56.0f, rect.bottom - 6.0f),
                     slot, iconColor);

        draw::Text(FontSize::Tiny, rect.left + 64.0f, rect.top + 6.0f, palette::kTextDim,
                   EquipSlotName(slot));
        if (equipped) {
            draw::Text(FontSize::Small, rect.left + 64.0f, rect.top + 28.0f, palette::kText,
                       equipped->DisplayName());
        } else {
            // 空きスロットは理由や、反対の手に装備中であることを表示する
            std::string reason = "未装備";
            ColorRGB reasonColor = palette::kTextDisabled;
            if (IsWeaponSlot(slot)) {
                const EquipSlot other = OppositeWeaponSlot(slot);
                if (inventory.Equipped(other) != nullptr) {
                    // 二刀流が無い間は片手にしか持てないので、どちらに持っているかを示す
                    reason = str::Format("%s に装備中", EquipSlotName(other));
                    reasonColor = palette::kTextDim;
                }
            } else if (slot == EquipSlot::Shield && inventory.IsDualWielding()) {
                reason = "二刀流中は装備不可";
            }
            draw::Text(FontSize::Small, rect.left + 64.0f, rect.top + 28.0f, reasonColor, reason);
        }
    }

    // スキルはスキルツリータブで設定する
    if (!slotButtons_.empty()) {
        const Rect last = slotButtons_.back().GetRect();
        draw::Text(FontSize::Tiny, last.left, last.bottom + 18.0f, palette::kTextDim,
                   "ソードスキルは「スキル」タブで装備します");
    }
}

void EquipPanel::DrawItemList(const GameContext& context) const
{
    const Inventory& inventory = context.player.GetInventory();
    const std::vector<const EquipmentItem*> items = inventory.ItemsForSlot(selectedSlot_);

    const float listLeft = window_.left + 372.0f;
    const Rect listArea(listLeft, window_.top + 90.0f, listLeft + 700.0f,
                        window_.top + 90.0f + kRowHeight * static_cast<float>(kVisibleRows));

    draw::FillRect(listArea.Expanded(6.0f), palette::kPanelDark, 190);
    draw::StrokeRect(listArea.Expanded(6.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    if (items.empty()) {
        draw::Text(FontSize::Normal, listArea.CenterX(), listArea.CenterY(), palette::kTextDisabled,
                   "所持している装備がありません", draw::TextAlign::Center);
        return;
    }

    for (int row = 0; row < kVisibleRows; ++row) {
        const int index = scroll_ + row;
        if (index >= static_cast<int>(items.size())) break;

        const EquipmentItem& item = *items[static_cast<size_t>(index)];
        const Rect rect(listArea.left, listArea.top + kRowHeight * static_cast<float>(row),
                        listArea.right, listArea.top + kRowHeight * static_cast<float>(row + 1) - 6.0f);
        DrawItemRow(rect, item, item.uid == selectedUid_, inventory.IsEquipped(item.uid), false);
    }

    // スクロールバー
    if (static_cast<int>(items.size()) > kVisibleRows) {
        const Rect track(listArea.right + 10.0f, listArea.top, listArea.right + 18.0f, listArea.bottom);
        draw::FillRect(track, palette::kPanelDark, 200);
        const float ratio = static_cast<float>(kVisibleRows) / static_cast<float>(items.size());
        const float offset = static_cast<float>(scroll_) / static_cast<float>(items.size());
        draw::FillRect(Rect(track.left, track.top + track.Height() * offset, track.right,
                            track.top + track.Height() * (offset + ratio)),
                       palette::kAccent, 220);
    }

    draw::Text(FontSize::Tiny, listArea.left, listArea.bottom + 14.0f, palette::kTextDim,
               str::Format("%s : %d 件（ホイールでスクロール）", EquipSlotName(selectedSlot_),
                           static_cast<int>(items.size())));
}

void EquipPanel::DrawComparison(const GameContext& context) const
{
    const Inventory& inventory = context.player.GetInventory();
    const Rect detail(window_.left + 1100.0f, window_.top + 90.0f, window_.right - 40.0f,
                      window_.bottom - 100.0f);

    draw::FillRect(detail, palette::kPanelDark, 205);
    draw::StrokeRect(detail, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    // --- キャラクタープレビュー ------------------------------------------------
    const Rect preview(detail.left + 20.0f, detail.top + 16.0f, detail.right - 20.0f, detail.top + 280.0f);
    draw::GradientRectV(preview, palette::kPanel, palette::kPanelDark, 220, 12);
    draw::StrokeRect(preview, palette::kBorder.Scaled(0.5f), 1.0f, 150);
    {
        // 装備スキンをそのまま反映したプレビュー
        const ActorArt art = inventory.BuildAppearance();

        const Rect body = Rect::FromFoot(preview.CenterX(), preview.bottom - 24.0f, 52.0f, 210.0f);
        DrawActorShadow(preview.CenterX(), preview.bottom - 24.0f, 120.0f, 70);
        DrawActor(body, 1, PoseKind::Idle, 0.25f, art, nullptr, 255, 0.0f);
    }

    const Stats current = context.player.TotalStats();

    // 選択中のアイテムを今のスロットへ装備した場合のステータス
    Stats next = current;
    const EquipmentItem* selected = inventory.FindByUid(selectedUid_);
    if (selected && inventory.CanEquipTo(selectedUid_, selectedSlot_)) {
        next = context.player.BaseStats() + inventory.PreviewStats(selectedUid_, selectedSlot_);
    }

    float y = preview.bottom + 22.0f;
    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "ステータス");
    draw::Text(FontSize::Normal, detail.right - 24.0f, y, palette::kText,
               str::Format("戦力 %d", current.Power()), draw::TextAlign::Right);
    y += 44.0f;

    DrawStatDiffLine(detail.left + 24.0f, y, "攻撃力", current.attack, next.attack, false); y += 32.0f;
    DrawStatDiffLine(detail.left + 24.0f, y, "防御力", current.defense, next.defense, false); y += 32.0f;
    DrawStatDiffLine(detail.left + 24.0f, y, "最大HP", current.maxHp, next.maxHp, false); y += 32.0f;
    DrawStatDiffLine(detail.left + 24.0f, y, "最大MP", current.maxMp, next.maxMp, false); y += 32.0f;
    DrawStatDiffLine(detail.left + 24.0f, y, "クリティカル率", current.critRate, next.critRate, true); y += 32.0f;
    DrawStatDiffLine(detail.left + 24.0f, y, "クリティカル倍率", current.critDamage, next.critDamage, true); y += 32.0f;
    DrawStatDiffLine(detail.left + 24.0f, y, "移動速度", current.moveSpeed, next.moveSpeed, false); y += 32.0f;
    DrawStatDiffLine(detail.left + 24.0f, y, "攻撃速度", current.attackSpeed, next.attackSpeed, true); y += 40.0f;

    if (selected) {
        draw::Line(detail.left + 24.0f, y, detail.right - 24.0f, y, palette::kBorder, 1.0f, 120);
        y += 14.0f;
        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kAccent,
                   selected->DisplayName());
        y += 26.0f;
        draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kTextDim,
                   str::Format("強化上限 +%d ／ 耐久力 最大 %d",
                               selected->MaxUpgrade(), selected->MaxDurabilityDisplay()));
        y += 22.0f;
        draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kTextDim, selected->flavor);
    }
}

} // namespace ui
} // namespace ecl
