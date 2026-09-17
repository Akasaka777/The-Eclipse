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

    // --- 左列: 装備スロット ---------------------------------------------------
    slotButtons_.clear();
    float y = window_.top + 84.0f;
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        slotButtons_.push_back(Button(Rect::FromXYWH(window_.left + 36.0f, y, 300.0f, 50.0f), ""));
        y += 58.0f;
    }

    // --- 左列: スキルスロット -------------------------------------------------
    skillSlotButtons_.clear();
    y += 36.0f;
    for (int i = 0; i < kSkillSlotCount; ++i) {
        skillSlotButtons_.push_back(Button(Rect::FromXYWH(window_.left + 36.0f, y, 300.0f, 50.0f), ""));
        y += 58.0f;
    }

    const float listLeft = window_.left + 372.0f;
    equipButton_ = Button(Rect::FromXYWH(listLeft, window_.bottom - 82.0f, 180.0f, 54.0f), "装備する");
    unequipButton_ = Button(Rect::FromXYWH(listLeft + 196.0f, window_.bottom - 82.0f, 180.0f, 54.0f), "外す");
    sellButton_ = Button(Rect::FromXYWH(listLeft + 392.0f, window_.bottom - 82.0f, 180.0f, 54.0f), "売却");
    sellButton_.SetAccent(palette::kAccentWarm);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 82.0f, 160.0f, 54.0f),
                          "閉じる");
}

std::vector<const SwordSkill*> EquipPanel::AvailableSkills(const GameContext& context) const
{
    std::vector<const SwordSkill*> result;
    const WeaponType weapon = context.player.CurrentWeaponType();
    for (const SwordSkill* skill : SkillDatabase::Instance().ForWeapon(weapon)) {
        if (context.player.IsSkillUnlocked(skill->id)) result.push_back(skill);
    }
    return result;
}

void EquipPanel::Open()
{
    open_ = true;
    closeRequested_ = false;
    selectedUid_ = 0;
    selectedSkillId_ = 0;
    skillSlot_ = -1;
    scroll_ = 0;
    message_.clear();
    messageTimer_ = 0.0f;
}

void EquipPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    Inventory& inventory = context.player.GetInventory();

    // --- 装備スロット選択 ------------------------------------------------------
    for (int i = 0; i < static_cast<int>(slotButtons_.size()); ++i) {
        const EquipSlot slot = static_cast<EquipSlot>(i);
        slotButtons_[static_cast<size_t>(i)].SetSelected(!SkillMode() && slot == selectedSlot_);
        if (slotButtons_[static_cast<size_t>(i)].Update(input, dt)) {
            selectedSlot_ = slot;
            skillSlot_ = -1;
            selectedUid_ = 0;
            scroll_ = 0;
        }
    }

    // --- スキルスロット選択 ----------------------------------------------------
    for (int i = 0; i < static_cast<int>(skillSlotButtons_.size()); ++i) {
        skillSlotButtons_[static_cast<size_t>(i)].SetSelected(skillSlot_ == i);
        if (skillSlotButtons_[static_cast<size_t>(i)].Update(input, dt)) {
            skillSlot_ = i;
            selectedSkillId_ = 0;
            scroll_ = 0;
        }
    }

    const float listLeft = window_.left + 372.0f;
    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());

    if (SkillMode()) {
        // ================= スキル装備モード =================
        const std::vector<const SwordSkill*> skills = AvailableSkills(context);
        const int maxScroll = math::MaxI(0, static_cast<int>(skills.size()) - kVisibleRows);

        const Rect listArea(listLeft, window_.top + 90.0f, listLeft + 700.0f,
                            window_.top + 90.0f + kRowHeight * static_cast<float>(kVisibleRows));
        if (listArea.Contains(mouseX, mouseY)) {
            const int wheel = input.WheelDelta();
            if (wheel != 0) scroll_ = math::ClampInt(scroll_ - wheel, 0, maxScroll);

            if (input.MouseClicked(MouseButton::Left)) {
                const int index = scroll_ + static_cast<int>((mouseY - listArea.top) / kRowHeight);
                if (index >= 0 && index < static_cast<int>(skills.size())) {
                    selectedSkillId_ = skills[static_cast<size_t>(index)]->id;
                }
            }
        }
        scroll_ = math::ClampInt(scroll_, 0, maxScroll);

        equipButton_.SetEnabled(selectedSkillId_ != 0);
        unequipButton_.SetEnabled(context.player.SkillAt(skillSlot_) != nullptr);
        sellButton_.SetEnabled(false);

        if (equipButton_.Update(input, dt) && equipButton_.Enabled()) {
            if (context.player.SetSkillAt(skillSlot_, selectedSkillId_)) {
                const SwordSkill* skill = SkillDatabase::Instance().Find(selectedSkillId_);
                message_ = str::Format("スロット %d に「%s」を装備しました", skillSlot_ + 1,
                                       skill ? skill->name.c_str() : "");
                messageTimer_ = 2.0f;
            }
        }
        if (unequipButton_.Update(input, dt) && unequipButton_.Enabled()) {
            context.player.ClearSkillSlot(skillSlot_);
            message_ = str::Format("スロット %d を空にしました", skillSlot_ + 1);
            messageTimer_ = 2.0f;
        }
        sellButton_.Update(input, dt);
    } else {
        // ================= 装備品モード =================
        const std::vector<const EquipmentItem*> items = inventory.ItemsForSlot(selectedSlot_);
        const int maxScroll = math::MaxI(0, static_cast<int>(items.size()) - kVisibleRows);

        const Rect listArea(listLeft, window_.top + 90.0f, listLeft + 700.0f,
                            window_.top + 90.0f + kRowHeight * static_cast<float>(kVisibleRows));
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

        const EquipmentItem* selected = inventory.FindByUid(selectedUid_);
        equipButton_.SetEnabled(selected != nullptr && !inventory.IsEquipped(selectedUid_));
        unequipButton_.SetEnabled(inventory.EquippedUid(selectedSlot_) != 0);
        sellButton_.SetEnabled(selected != nullptr && !inventory.IsEquipped(selectedUid_));

        if (equipButton_.Update(input, dt) && equipButton_.Enabled()) {
            if (inventory.Equip(selectedUid_)) {
                context.player.RefreshSkillLoadout();
                message_ = "装備を変更しました";
                messageTimer_ = 2.0f;
            }
        }
        if (unequipButton_.Update(input, dt) && unequipButton_.Enabled()) {
            inventory.Unequip(selectedSlot_);
            context.player.RefreshSkillLoadout();
            message_ = "装備を外しました";
            messageTimer_ = 2.0f;
        }
        if (sellButton_.Update(input, dt) && sellButton_.Enabled()) {
            const int value = inventory.SellValue(selectedUid_);
            if (inventory.Sell(selectedUid_)) {
                message_ = str::Format("売却しました（+%s col）", str::Comma(value).c_str());
                messageTimer_ = 2.0f;
                selectedUid_ = 0;
            }
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

    DrawWindow(window_, SkillMode() ? "装備 － ソードスキル" : "装備");
    DrawSlotColumn(context);

    if (SkillMode()) {
        DrawSkillList(context);
        DrawSkillDetail(context);
    } else {
        DrawItemList(context);
        DrawComparison(context);
    }

    equipButton_.Draw();
    unequipButton_.Draw();
    if (!SkillMode()) sellButton_.Draw();
    closeButton_.Draw();

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, window_.left + 372.0f, window_.bottom - 112.0f, palette::kAccent,
                   message_);
    }
}

void EquipPanel::DrawSlotColumn(const GameContext& context) const
{
    const Inventory& inventory = context.player.GetInventory();

    // --- 装備スロット ---------------------------------------------------------
    for (int i = 0; i < static_cast<int>(slotButtons_.size()); ++i) {
        const EquipSlot slot = static_cast<EquipSlot>(i);
        const Rect rect = slotButtons_[static_cast<size_t>(i)].GetRect();
        const bool selected = (!SkillMode() && slot == selectedSlot_);
        const EquipmentItem* equipped = inventory.Equipped(slot);

        const ColorRGB base = selected ? palette::kPanelLight : palette::kPanelDark;
        draw::GradientRectH(rect, base, base.Scaled(0.7f), 235, 10);
        draw::StrokeRect(rect, selected ? palette::kAccent : palette::kBorder.Scaled(0.6f),
                         selected ? 3.0f : 1.0f, 255);

        const ColorRGB iconColor = equipped ? RarityColor(equipped->rarity) : palette::kTextDisabled;
        DrawSlotIcon(Rect(rect.left + 6.0f, rect.top + 6.0f, rect.left + 50.0f, rect.bottom - 6.0f),
                     slot, iconColor);

        draw::Text(FontSize::Tiny, rect.left + 58.0f, rect.top + 4.0f, palette::kTextDim,
                   EquipSlotName(slot));
        if (equipped) {
            draw::Text(FontSize::Small, rect.left + 58.0f, rect.top + 22.0f, palette::kText,
                       equipped->DisplayName());
        } else {
            draw::Text(FontSize::Small, rect.left + 58.0f, rect.top + 22.0f, palette::kTextDisabled,
                       "未装備");
        }
    }

    // --- スキルスロット -------------------------------------------------------
    if (skillSlotButtons_.empty()) return;

    const Rect first = skillSlotButtons_[0].GetRect();
    draw::Line(first.left, first.top - 18.0f, first.right, first.top - 18.0f,
               palette::kBorder, 1.0f, 140);
    draw::Text(FontSize::Small, first.left, first.top - 40.0f, palette::kAccent,
               str::Format("ソードスキル（%s）", WeaponTypeName(context.player.CurrentWeaponType())));

    for (int i = 0; i < static_cast<int>(skillSlotButtons_.size()); ++i) {
        const Rect rect = skillSlotButtons_[static_cast<size_t>(i)].GetRect();
        const bool selected = (skillSlot_ == i);
        const SwordSkill* skill = context.player.SkillAt(i);

        const ColorRGB base = selected ? palette::kPanelLight : palette::kPanelDark;
        draw::GradientRectH(rect, base, base.Scaled(0.7f), 235, 10);
        draw::StrokeRect(rect, selected ? palette::kAccent : palette::kBorder.Scaled(0.6f),
                         selected ? 3.0f : 1.0f, 255);

        // スロット番号
        const Rect keyTag = Rect::FromXYWH(rect.left + 6.0f, rect.top + 10.0f, 30.0f, 28.0f);
        draw::FillRect(keyTag, palette::kBlack, 180);
        draw::Text(FontSize::Tiny, keyTag.CenterX(), keyTag.top + 5.0f, palette::kText,
                   str::Format("%d", i + 1), draw::TextAlign::Center);

        if (skill) {
            draw::Text(FontSize::Small, rect.left + 46.0f, rect.top + 6.0f, palette::kText, skill->name);
            draw::Text(FontSize::Tiny, rect.left + 46.0f, rect.top + 30.0f, palette::kTextDim,
                       str::Format("MP %d  CD %.1fs", static_cast<int>(skill->mpCost), skill->cooldown));
        } else {
            draw::Text(FontSize::Small, rect.left + 46.0f, rect.top + 14.0f, palette::kTextDisabled,
                       "スキル未装備");
        }
    }
}

void EquipPanel::DrawSkillList(const GameContext& context) const
{
    const std::vector<const SwordSkill*> skills = AvailableSkills(context);

    const float listLeft = window_.left + 372.0f;
    const Rect listArea(listLeft, window_.top + 90.0f, listLeft + 700.0f,
                        window_.top + 90.0f + kRowHeight * static_cast<float>(kVisibleRows));

    draw::FillRect(listArea.Expanded(6.0f), palette::kPanelDark, 190);
    draw::StrokeRect(listArea.Expanded(6.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    if (skills.empty()) {
        draw::Text(FontSize::Normal, listArea.CenterX(), listArea.CenterY() - 20.0f,
                   palette::kTextDisabled, "解放済みのスキルがありません", draw::TextAlign::Center);
        draw::Text(FontSize::Small, listArea.CenterX(), listArea.CenterY() + 16.0f,
                   palette::kTextDisabled, "スキルツリータブで解放してください", draw::TextAlign::Center);
        return;
    }

    for (int row = 0; row < kVisibleRows; ++row) {
        const int index = scroll_ + row;
        if (index >= static_cast<int>(skills.size())) break;

        const SwordSkill& skill = *skills[static_cast<size_t>(index)];
        const Rect rect(listArea.left, listArea.top + kRowHeight * static_cast<float>(row),
                        listArea.right, listArea.top + kRowHeight * static_cast<float>(row + 1) - 6.0f);

        const bool selected = (skill.id == selectedSkillId_);
        const int equippedSlot = context.player.SkillSlotOf(skill.id);

        ColorRGB fill = palette::kPanelDark;
        if (selected) fill = ColorRGB::Lerp(palette::kPanelLight, skill.effectColor.Scaled(0.5f), 0.55f);
        draw::GradientRectH(rect, fill, fill.Scaled(0.75f), 235, 12);
        draw::StrokeRect(rect, selected ? skill.effectColor : palette::kBorder.Scaled(0.7f),
                         selected ? 2.0f : 1.0f, 255);
        draw::FillRect(Rect(rect.left, rect.top, rect.left + 6.0f, rect.bottom), skill.effectColor, 255);

        DrawWeaponIcon(Rect(rect.left + 12.0f, rect.top + 6.0f, rect.left + 62.0f, rect.bottom - 6.0f),
                       skill.weapon, skill.effectColor);

        draw::Text(FontSize::Normal, rect.left + 74.0f, rect.top + 8.0f, palette::kText, skill.name);
        draw::Text(FontSize::Small, rect.left + 74.0f, rect.top + 36.0f, palette::kTextDim,
                   str::Format("MP %d   CD %.1fs   %d段   合計 %.0f%%",
                               static_cast<int>(skill.mpCost), skill.cooldown,
                               static_cast<int>(skill.strikes.size()),
                               skill.TotalMultiplier() * 100.0f));

        if (equippedSlot >= 0) {
            draw::FillRect(Rect(rect.right - 92.0f, rect.top + 8.0f, rect.right - 12.0f, rect.top + 32.0f),
                           palette::kAccent, 225);
            draw::Text(FontSize::Tiny, rect.right - 52.0f, rect.top + 11.0f, palette::kBlack,
                       str::Format("スロット%d", equippedSlot + 1), draw::TextAlign::Center);
        }
    }

    draw::Text(FontSize::Tiny, listArea.left, listArea.bottom + 14.0f, palette::kTextDim,
               str::Format("解放済み %d 件 ／ スロット %d に装備します",
                           static_cast<int>(skills.size()), skillSlot_ + 1));
}

void EquipPanel::DrawSkillDetail(const GameContext& context) const
{
    const Rect detail(window_.left + 1100.0f, window_.top + 90.0f, window_.right - 40.0f,
                      window_.bottom - 100.0f);
    draw::FillRect(detail, palette::kPanelDark, 205);
    draw::StrokeRect(detail, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    const SwordSkill* skill = SkillDatabase::Instance().Find(selectedSkillId_);
    if (!skill) {
        draw::Text(FontSize::Normal, detail.CenterX(), detail.CenterY() - 40.0f, palette::kTextDisabled,
                   "スキルを選択してください", draw::TextAlign::Center);
        return;
    }

    float y = detail.top + 24.0f;
    draw::Text(FontSize::Large, detail.left + 24.0f, y, skill->effectColor, skill->name);
    y += 58.0f;
    draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, skill->description);
    y += 48.0f;

    draw::Line(detail.left + 24.0f, y, detail.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;

    struct Row { const char* label; std::string value; };
    const Row rows[] = {
        { "消費 MP",      str::Format("%d", static_cast<int>(skill->mpCost)) },
        { "クールダウン", str::Format("%.1f 秒", skill->cooldown) },
        { "モーション",   str::Format("%.2f 秒", skill->duration) },
        { "ヒット数",     str::Format("%d 段", static_cast<int>(skill->strikes.size())) },
        { "合計倍率",     str::Format("%.0f%%", skill->TotalMultiplier() * 100.0f) },
        { "無敵時間",     skill->invincibleUntil > 0.0f
                              ? str::Format("%.2f 秒", skill->invincibleUntil) : "なし" },
    };
    for (const Row& row : rows) {
        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, row.label);
        draw::Text(FontSize::Small, detail.right - 24.0f, y, palette::kText, row.value,
                   draw::TextAlign::Right);
        y += 34.0f;
    }

    y += 12.0f;
    const int equippedSlot = context.player.SkillSlotOf(skill->id);
    if (equippedSlot >= 0) {
        draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent,
                   str::Format("スロット %d に装備中", equippedSlot + 1));
    } else {
        draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kTextDim, "未装備");
    }
    y += 44.0f;

    draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kTextDim,
               "戦闘中は対応する番号キー、または");
    y += 26.0f;
    draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kTextDim,
               "画面右下のアイコンのクリックで発動します。");
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
        ActorArt art;
        art.style = ArtStyle::Humanoid;
        art.main = ColorRGB(48, 58, 84);
        art.accent = ColorRGB(226, 234, 248);
        art.trim = palette::kAccent;
        art.weapon = context.player.CurrentWeaponType();
        art.hasWeapon = inventory.Equipped(EquipSlot::Weapon) != nullptr;
        art.hasShield = inventory.Equipped(EquipSlot::Shield) != nullptr;

        const Rect body = Rect::FromFoot(preview.CenterX(), preview.bottom - 24.0f, 52.0f, 210.0f);
        DrawActorShadow(preview.CenterX(), preview.bottom - 24.0f, 120.0f, 70);
        DrawActor(body, 1, PoseKind::Idle, 0.25f, art, nullptr, 255, 0.0f);
    }

    const Stats current = context.player.TotalStats();

    // 選択中のアイテムを装備した場合のステータス
    Stats next = current;
    const EquipmentItem* selected = inventory.FindByUid(selectedUid_);
    if (selected && !inventory.IsEquipped(selectedUid_)) {
        const EquipmentItem* equipped = inventory.Equipped(selected->slot);
        if (equipped) next += selected->TotalStats().Scaled(1.0f) + equipped->TotalStats().Scaled(-1.0f);
        else next += selected->TotalStats();
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
        draw::Text(FontSize::Small, detail.left + 24.0f, y, RarityColor(selected->rarity),
                   selected->DisplayName());
        y += 30.0f;
        draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kTextDim, selected->flavor);
    }
}

} // namespace ui
} // namespace ecl
