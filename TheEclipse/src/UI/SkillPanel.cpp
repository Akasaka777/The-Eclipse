#include "UI/SkillPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Game/UniqueSkill.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {
namespace ui {

namespace {

constexpr float kNodeWidth = 280.0f;
constexpr float kNodeHeight = 110.0f;
constexpr float kColumnGap = 360.0f;
constexpr float kTierGap = 150.0f;

const char* ColumnName(int column)
{
    return (column == 0) ? "火力系統" : "機動・範囲系統";
}

} // namespace

SkillPanel::SkillPanel()
{
    Layout();
}

void SkillPanel::Layout()
{
    window_ = Rect::FromXYWH(200.0f, 100.0f, 1520.0f, 880.0f);

    // 武器種の切り替えタブ
    weaponButtons_.clear();
    const float tabWidth = 220.0f;
    const float tabGap = 14.0f;
    for (int i = 0; i < static_cast<int>(WeaponType::Count); ++i) {
        const float x = window_.left + 40.0f + (tabWidth + tabGap) * static_cast<float>(i);
        weaponButtons_.push_back(Button(Rect::FromXYWH(x, window_.top + 70.0f, tabWidth, 46.0f),
                                        WeaponTypeName(static_cast<WeaponType>(i)), FontSize::Small));
    }

    treeArea_ = Rect(window_.left + 40.0f, window_.top + 140.0f, window_.left + 700.0f,
                     window_.top + 580.0f);
    detailArea_ = Rect(window_.left + 730.0f, window_.top + 140.0f, window_.right - 40.0f,
                       window_.top + 580.0f);

    // 装備スロット
    const float slotWidth = 280.0f;
    const float slotGap = 20.0f;
    const float slotsWidth = slotWidth * kSkillSlotCount + slotGap * (kSkillSlotCount - 1);
    const float slotStart = window_.CenterX() - slotsWidth * 0.5f;
    for (int i = 0; i < kSkillSlotCount; ++i) {
        slotRects_[i] = Rect::FromXYWH(slotStart + (slotWidth + slotGap) * static_cast<float>(i),
                                       window_.top + 620.0f, slotWidth, 116.0f);
    }

    // ユニークスキルのタブ（武器タブの右端に置く）
    uniqueTabButton_ = Button(Rect::FromXYWH(window_.right - 250.0f, window_.top + 70.0f,
                                             210.0f, 46.0f), "ユニーク", FontSize::Small);
    uniqueTabButton_.SetAccent(palette::kExp);
    acquireButton_ = Button(Rect::FromXYWH(detailArea_.left + 20.0f, detailArea_.bottom - 146.0f,
                                           460.0f, 56.0f), "このユニークスキルを習得する",
                            FontSize::Small);
    acquireButton_.SetAccent(palette::kExp);

    unlockButton_ = Button(Rect::FromXYWH(detailArea_.left + 20.0f, detailArea_.bottom - 76.0f,
                                          220.0f, 56.0f), "解放する");
    unlockButton_.SetAccent(palette::kExp);
    equipButton_ = Button(Rect::FromXYWH(detailArea_.left + 260.0f, detailArea_.bottom - 76.0f,
                                         220.0f, 56.0f), "スロットへ装備");
    unequipButton_ = Button(Rect::FromXYWH(detailArea_.left + 500.0f, detailArea_.bottom - 76.0f,
                                           180.0f, 56.0f), "外す");
    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 86.0f,
                                         160.0f, 58.0f), "閉じる");
}

Rect SkillPanel::NodeRect(int column, int tier) const
{
    const float cx = treeArea_.left + 150.0f + kColumnGap * static_cast<float>(column);
    const float cy = treeArea_.top + 60.0f + kTierGap * static_cast<float>(tier - 1);
    return Rect::FromCenter(cx, cy, kNodeWidth, kNodeHeight);
}

void SkillPanel::Open(const GameContext& context)
{
    open_ = true;
    closeRequested_ = false;
    viewWeapon_ = context.player.CurrentWeaponType();
    uniqueTab_ = false;
    selectedSkillId_ = 0;
    targetSlot_ = 0;
    message_.clear();
    messageTimer_ = 0.0f;
}

UniqueSkillType SkillPanel::VisibleUniqueType(const GameContext& context) const
{
    // 習得済みがあればそれを、無ければ解放条件を満たしたものを表示する
    if (context.player.HasUniqueSkill()) return context.player.UniqueSkill();
    for (const UniqueSkillDef& def : UniqueSkillDatabase::Instance().All()) {
        if (context.player.IsUniqueSkillAvailable(def.type)) return def.type;
    }
    return UniqueSkillType::None;
}

bool SkillPanel::HasVisibleUnique(const GameContext& context) const
{
    return VisibleUniqueType(context) != UniqueSkillType::None;
}

bool SkillPanel::IsCurrentWeapon(const GameContext& context) const
{
    return viewWeapon_ == context.player.CurrentWeaponType();
}

void SkillPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;

    closeRequested_ = false;
    time_ += dt;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    PlayerData& player = context.player;

    // --- 武器種タブ ----------------------------------------------------------
    for (int i = 0; i < static_cast<int>(weaponButtons_.size()); ++i) {
        const bool selected = (!uniqueTab_ && static_cast<WeaponType>(i) == viewWeapon_);
        weaponButtons_[static_cast<size_t>(i)].SetSelected(selected);
        if (weaponButtons_[static_cast<size_t>(i)].Update(input, dt)) {
            viewWeapon_ = static_cast<WeaponType>(i);
            uniqueTab_ = false;
            selectedSkillId_ = 0;
        }
    }

    // --- ユニークスキルのタブ（解放条件を満たすまで表示しない） ------------------
    const bool uniqueVisible = HasVisibleUnique(context);
    if (uniqueVisible) {
        uniqueTabButton_.SetSelected(uniqueTab_);
        if (uniqueTabButton_.Update(input, dt)) {
            uniqueTab_ = true;
            selectedSkillId_ = 0;
        }
    } else {
        uniqueTab_ = false;
    }

    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());

    // --- ノード選択 ----------------------------------------------------------
    if (input.MouseClicked(MouseButton::Left)) {
        if (uniqueTab_) {
            const std::vector<const SwordSkill*> nodes =
                SkillDatabase::Instance().ForUnique(VisibleUniqueType(context));
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (NodeRect(0, static_cast<int>(i) + 1).Contains(mouseX, mouseY)) {
                    selectedSkillId_ = nodes[i]->id;
                }
            }
        } else {
        for (int column = 0; column < kSkillTreeColumns; ++column) {
            for (int tier = 1; tier <= kSkillTreeTiers; ++tier) {
                const SwordSkill* node = SkillDatabase::Instance().NodeAt(viewWeapon_, column, tier);
                if (!node) continue;
                if (NodeRect(column, tier).Contains(mouseX, mouseY)) {
                    selectedSkillId_ = node->id;
                }
            }
        }
        }
        // --- スロット選択 ----------------------------------------------------
        for (int i = 0; i < player.SkillSlotLimit(); ++i) {
            if (slotRects_[i].Contains(mouseX, mouseY)) targetSlot_ = i;
        }
    }

    const SwordSkill* selected = SkillDatabase::Instance().Find(selectedSkillId_);
    const bool unlocked = selected && player.IsSkillUnlocked(selected->id);
    const bool canUnlock = selected && player.CanUnlockSkill(selected->id);
    const bool equippable = unlocked && IsCurrentWeapon(context);

    unlockButton_.SetEnabled(canUnlock);
    equipButton_.SetEnabled(equippable);
    if (targetSlot_ >= player.SkillSlotLimit()) targetSlot_ = 0;
    unequipButton_.SetEnabled(player.SkillAt(targetSlot_) != nullptr);

    // --- ユニークスキルの習得 --------------------------------------------------
    const UniqueSkillType uniqueType = VisibleUniqueType(context);
    const bool canAcquire = uniqueTab_ && uniqueType != UniqueSkillType::None
                         && (!player.HasUniqueSkill() || context.settings.debugMode);
    acquireButton_.SetEnabled(canAcquire);
    if (uniqueTab_ && acquireButton_.Update(input, dt) && canAcquire) {
        // デバッグモードでは習得済みでも切り替えられる
        if (player.AcquireUniqueSkill(uniqueType, context.settings.debugMode)) {
            message_ = str::Format("ユニークスキル「%s」を習得しました",
                                   UniqueSkillName(uniqueType));
            messageTimer_ = 3.0f;
        }
    }

    if (unlockButton_.Update(input, dt) && canUnlock) {
        if (player.UnlockSkill(selected->id)) {
            message_ = str::Format("「%s」を解放しました", selected->name.c_str());
            messageTimer_ = 2.4f;
        }
    }
    if (equipButton_.Update(input, dt) && equippable) {
        if (player.SetSkillAt(targetSlot_, selected->id)) {
            message_ = str::Format("スロット %d に「%s」を装備しました",
                                   targetSlot_ + 1, selected->name.c_str());
            messageTimer_ = 2.4f;
            // 次の空きスロットへ自動で移動する
            const int limit = player.SkillSlotLimit();
            for (int i = 1; i <= limit; ++i) {
                const int next = (targetSlot_ + i) % limit;
                if (player.SkillAt(next) == nullptr) {
                    targetSlot_ = next;
                    break;
                }
            }
        }
    }
    if (unequipButton_.Update(input, dt) && unequipButton_.Enabled()) {
        player.ClearSkillSlot(targetSlot_);
        message_ = str::Format("スロット %d を空にしました", targetSlot_ + 1);
        messageTimer_ = 2.4f;
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void SkillPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "スキルツリー");

    // スキルポイント
    const Rect spTag = Rect::FromXYWH(window_.right - 280.0f, window_.top + 8.0f, 240.0f, 40.0f);
    draw::FillRect(spTag, palette::kExp.Scaled(0.35f), 230);
    draw::StrokeRect(spTag, palette::kExp, 2.0f, 255);
    draw::Text(FontSize::Normal, spTag.CenterX(), spTag.top + 6.0f, palette::kText,
               str::Format("SP  %d", context.player.SkillPoints()), draw::TextAlign::Center);

    DrawWeaponTabs(context);
    if (uniqueTab_) DrawUniqueTree(context);
    else DrawTree(context);
    DrawDetail(context);
    DrawSlots(context);

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, window_.left + 40.0f, window_.bottom - 64.0f, palette::kAccent,
                   message_);
    }
    closeButton_.Draw();
}

void SkillPanel::DrawWeaponTabs(const GameContext& context) const
{
    // ユニークスキルは解放条件を満たすまで表示しない
    if (HasVisibleUnique(context)) {
        uniqueTabButton_.Draw();
        if (!context.player.HasUniqueSkill()) {
            const Rect rect = uniqueTabButton_.GetRect();
            const float pulse = 0.7f + 0.3f * std::sin(time_ * 4.0f);
            draw::StrokeRect(rect.Expanded(3.0f), palette::kExp, 2.0f,
                             static_cast<int>(200.0f * pulse));
            draw::Text(FontSize::Tiny, rect.CenterX(), rect.top - 20.0f, palette::kExp,
                       "習得可能！", draw::TextAlign::Center);
        }
    }

    for (int i = 0; i < static_cast<int>(weaponButtons_.size()); ++i) {
        weaponButtons_[static_cast<size_t>(i)].Draw();

        // 現在装備中の武器種に印を付ける
        if (static_cast<WeaponType>(i) == context.player.CurrentWeaponType()) {
            const Rect rect = weaponButtons_[static_cast<size_t>(i)].GetRect();
            draw::Text(FontSize::Tiny, rect.right - 8.0f, rect.top - 20.0f, palette::kAccent,
                       "装備中", draw::TextAlign::Right);
        }
    }
}

void SkillPanel::DrawTree(const GameContext& context) const
{
    const PlayerData& player = context.player;
    const SkillDatabase& database = SkillDatabase::Instance();

    draw::FillRect(treeArea_.Expanded(8.0f), palette::kPanelDark, 190);
    draw::StrokeRect(treeArea_.Expanded(8.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    // 系統名
    for (int column = 0; column < kSkillTreeColumns; ++column) {
        const Rect head = NodeRect(column, 1);
        draw::Text(FontSize::Tiny, head.CenterX(), treeArea_.top + 2.0f, palette::kTextDim,
                   ColumnName(column), draw::TextAlign::Center);
    }

    for (int column = 0; column < kSkillTreeColumns; ++column) {
        for (int tier = 1; tier <= kSkillTreeTiers; ++tier) {
            const SwordSkill* node = database.NodeAt(viewWeapon_, column, tier);
            if (!node) continue;

            const Rect rect = NodeRect(column, tier);
            const bool unlocked = player.IsSkillUnlocked(node->id);
            const bool reachable = player.IsSkillReachable(node->id);
            const bool canUnlock = player.CanUnlockSkill(node->id);
            const bool selected = (node->id == selectedSkillId_);
            const int equippedSlot = player.SkillSlotOf(node->id);
            const bool equipped = equippedSlot >= 0 && IsCurrentWeapon(context);

            // 前提スキルとの接続線
            if (tier > 1) {
                const Rect parent = NodeRect(column, tier - 1);
                const ColorRGB lineColor = unlocked ? node->effectColor
                                         : (reachable ? palette::kBorder : palette::kTextDisabled);
                draw::Line(parent.CenterX(), parent.bottom, rect.CenterX(), rect.top,
                           lineColor, unlocked ? 4.0f : 2.0f, 220);
            }

            // ノード本体
            ColorRGB fill = palette::kPanelDark;
            if (unlocked) fill = ColorRGB::Lerp(palette::kPanelLight, node->effectColor.Scaled(0.5f), 0.55f);
            else if (canUnlock) fill = palette::kPanelLight;

            draw::GradientRectV(rect, fill.Scaled(1.15f), fill.Scaled(0.75f), 235, 10);

            ColorRGB border = palette::kTextDisabled;
            if (equipped) border = palette::kAccent;
            else if (unlocked) border = node->effectColor;
            else if (canUnlock) border = palette::kExp;
            draw::StrokeRect(rect, border, selected ? 4.0f : 2.0f, 255);

            if (selected) {
                const float pulse = 0.5f + 0.5f * std::sin(time_ * 5.0f);
                draw::StrokeRect(rect.Expanded(4.0f), palette::kWhite, 1.0f,
                                 static_cast<int>(150.0f * pulse));
            }

            // 武器アイコン
            DrawWeaponIcon(Rect(rect.left + 8.0f, rect.top + 8.0f, rect.left + 68.0f, rect.bottom - 8.0f),
                           node->weapon, unlocked ? node->effectColor : palette::kTextDisabled);

            const ColorRGB textColor = unlocked ? palette::kText
                                     : (reachable ? palette::kTextDim : palette::kTextDisabled);
            draw::Text(FontSize::Small, rect.left + 76.0f, rect.top + 10.0f, textColor, node->name);
            draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.top + 38.0f, palette::kTextDim,
                       str::Format("MP %d  CD %.1fs", static_cast<int>(node->mpCost), node->cooldown));

            // 状態表示
            if (equipped) {
                draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, palette::kAccent,
                           str::Format("装備中  スロット %d", equippedSlot + 1));
            } else if (unlocked) {
                draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, palette::kHp, "解放済み");
            } else if (!reachable) {
                draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, palette::kTextDisabled,
                           "前提スキルが必要");
            } else {
                const ColorRGB costColor = canUnlock ? palette::kExp : palette::kDanger;
                draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, costColor,
                           str::Format("解放に SP %d", node->unlockCost));
            }
        }
    }
}

void SkillPanel::DrawUniqueTree(const GameContext& context) const
{
    const PlayerData& player = context.player;
    const UniqueSkillType type = VisibleUniqueType(context);
    const UniqueSkillDef* def = UniqueSkillDatabase::Instance().Find(type);

    draw::FillRect(treeArea_.Expanded(8.0f), palette::kPanelDark, 190);
    draw::StrokeRect(treeArea_.Expanded(8.0f), palette::kExp.Scaled(0.7f), 1.0f, 180);

    if (!def) return;

    draw::Text(FontSize::Small, treeArea_.left + 12.0f, treeArea_.top + 2.0f, palette::kExp,
               str::Format("ユニークスキル : %s", def->name.c_str()));

    const bool acquired = (player.UniqueSkill() == type);
    const std::vector<const SwordSkill*> nodes = SkillDatabase::Instance().ForUnique(type);

    for (size_t i = 0; i < nodes.size(); ++i) {
        const SwordSkill* node = nodes[i];
        const Rect rect = NodeRect(0, static_cast<int>(i) + 1);

        const bool unlocked = player.IsSkillUnlocked(node->id);
        const bool reachable = acquired && player.IsSkillReachable(node->id);
        const bool canUnlock = player.CanUnlockSkill(node->id);
        const bool selected = (node->id == selectedSkillId_);
        const int equippedSlot = player.SkillSlotOf(node->id);

        if (i > 0) {
            const Rect parent = NodeRect(0, static_cast<int>(i));
            draw::Line(parent.CenterX(), parent.bottom, rect.CenterX(), rect.top,
                       unlocked ? node->effectColor : palette::kTextDisabled,
                       unlocked ? 4.0f : 2.0f, 220);
        }

        ColorRGB fill = palette::kPanelDark;
        if (unlocked) fill = ColorRGB::Lerp(palette::kPanelLight, node->effectColor.Scaled(0.5f), 0.55f);
        else if (canUnlock) fill = palette::kPanelLight;
        draw::GradientRectV(rect, fill.Scaled(1.15f), fill.Scaled(0.75f), 235, 10);

        ColorRGB border = palette::kTextDisabled;
        if (equippedSlot >= 0) border = palette::kAccent;
        else if (unlocked) border = node->effectColor;
        else if (canUnlock) border = palette::kExp;
        draw::StrokeRect(rect, border, selected ? 4.0f : 2.0f, 255);

        DrawWeaponIcon(Rect(rect.left + 8.0f, rect.top + 8.0f, rect.left + 68.0f, rect.bottom - 8.0f),
                       node->weapon, unlocked ? node->effectColor : palette::kTextDisabled);

        const ColorRGB textColor = unlocked ? palette::kText
                                 : (reachable ? palette::kTextDim : palette::kTextDisabled);
        draw::Text(FontSize::Small, rect.left + 76.0f, rect.top + 10.0f, textColor, node->name);
        draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.top + 38.0f, palette::kTextDim,
                   str::Format("MP %d  CD %.1fs", static_cast<int>(node->mpCost), node->cooldown));

        if (!acquired) {
            draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, palette::kTextDisabled,
                       "ユニークスキルの習得が必要");
        } else if (equippedSlot >= 0) {
            draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, palette::kAccent,
                       str::Format("装備中  スロット %d", equippedSlot + 1));
        } else if (unlocked) {
            draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, palette::kHp, "解放済み");
        } else if (!reachable) {
            draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f, palette::kTextDisabled,
                       "前提スキルが必要");
        } else {
            draw::Text(FontSize::Tiny, rect.left + 76.0f, rect.bottom - 26.0f,
                       canUnlock ? palette::kExp : palette::kDanger,
                       str::Format("解放に SP %d", node->unlockCost));
        }
    }

    // 効果と解放条件の説明
    const float infoY = treeArea_.top + 470.0f;
    draw::Text(FontSize::Tiny, treeArea_.left + 12.0f, infoY, palette::kTextDim, def->description);
    draw::Text(FontSize::Tiny, treeArea_.left + 12.0f, infoY + 26.0f,
               acquired ? palette::kHp : palette::kExp,
               acquired ? "習得済み" : str::Format("解放条件 : %s", def->unlockCondition.c_str()));
}

void SkillPanel::DrawDetail(const GameContext& context) const
{
    const PlayerData& player = context.player;

    draw::FillRect(detailArea_, palette::kPanelDark, 205);
    draw::StrokeRect(detailArea_, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    const SwordSkill* skill = SkillDatabase::Instance().Find(selectedSkillId_);
    if (!skill) {
        draw::Text(FontSize::Normal, detailArea_.CenterX(), detailArea_.CenterY() - 40.0f,
                   palette::kTextDisabled, "スキルを選択してください", draw::TextAlign::Center);
        draw::Text(FontSize::Small, detailArea_.CenterX(), detailArea_.CenterY() + 4.0f,
                   palette::kTextDisabled, "ツリーのノードをクリック", draw::TextAlign::Center);
        return;
    }

    float y = detailArea_.top + 22.0f;
    draw::Text(FontSize::Large, detailArea_.left + 24.0f, y, skill->effectColor, skill->name);
    y += 58.0f;
    draw::Text(FontSize::Small, detailArea_.left + 24.0f, y, palette::kTextDim, skill->description);
    y += 44.0f;

    draw::Line(detailArea_.left + 24.0f, y, detailArea_.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;

    struct Row { const char* label; std::string value; };
    const Row rows[] = {
        { "武器種",       WeaponTypeName(skill->weapon) },
        { "消費 MP",      str::Format("%d", static_cast<int>(skill->mpCost)) },
        { "クールダウン", str::Format("%.1f 秒", skill->cooldown) },
        { "ヒット数",     str::Format("%d 段", static_cast<int>(skill->strikes.size())) },
        { "合計倍率",     str::Format("%.0f%%", skill->TotalMultiplier() * 100.0f) },
        { "無敵時間",     skill->invincibleUntil > 0.0f
                              ? str::Format("%.2f 秒", skill->invincibleUntil) : "なし" },
    };
    for (const Row& row : rows) {
        draw::Text(FontSize::Small, detailArea_.left + 24.0f, y, palette::kTextDim, row.label);
        draw::Text(FontSize::Small, detailArea_.left + 360.0f, y, palette::kText, row.value,
                   draw::TextAlign::Right);
        y += 32.0f;
    }
    y += 10.0f;

    // 解放状態
    if (player.IsSkillUnlocked(skill->id)) {
        const int slot = player.SkillSlotOf(skill->id);
        if (slot >= 0 && IsCurrentWeapon(context)) {
            draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y, palette::kAccent,
                       str::Format("スロット %d に装備中", slot + 1));
        } else {
            draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y, palette::kHp, "解放済み");
        }
    } else if (!player.IsSkillReachable(skill->id)) {
        const SwordSkill* parent = SkillDatabase::Instance().Find(skill->requiredSkillId);
        draw::Text(FontSize::Small, detailArea_.left + 24.0f, y, palette::kDanger,
                   str::Format("前提スキル : %s", parent ? parent->name.c_str() : "?"));
    } else {
        const bool enough = context.player.SkillPoints() >= skill->unlockCost;
        draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y,
                   enough ? palette::kExp : palette::kDanger,
                   str::Format("解放に必要な SP : %d", skill->unlockCost));
    }

    if (!IsCurrentWeapon(context)) {
        draw::Text(FontSize::Tiny, detailArea_.left + 24.0f, detailArea_.bottom - 104.0f,
                   palette::kTextDim, "※ 装備するには対応する武器を装備してください");
    }

    if (uniqueTab_) acquireButton_.Draw();
    unlockButton_.Draw();
    equipButton_.Draw();
    unequipButton_.Draw();
}

void SkillPanel::DrawSlots(const GameContext& context) const
{
    const PlayerData& player = context.player;

    const int limit = player.SkillSlotLimit();
    draw::Text(FontSize::Small, slotRects_[0].left, slotRects_[0].top - 30.0f, palette::kAccent,
               str::Format("装備スキル（最大 %d つ / クリックで装備先を選択）", limit));
    if (player.HasUniqueSkill()) {
        draw::Text(FontSize::Tiny, slotRects_[kSkillSlotCount - 1].right, slotRects_[0].top - 28.0f,
                   palette::kExp,
                   str::Format("ユニークスキル「%s」習得中のため 1 枠減少",
                               UniqueSkillName(player.UniqueSkill())),
                   draw::TextAlign::Right);
    }

    for (int i = 0; i < kSkillSlotCount; ++i) {
        const Rect rect = slotRects_[i];
        if (i >= limit) {
            // 使えない枠は封鎖表示
            draw::FillRect(rect, palette::kPanelDark, 200);
            draw::StrokeRect(rect, palette::kTextDisabled, 1.0f, 180);
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 12.0f,
                       palette::kTextDisabled, "使用不可", draw::TextAlign::Center);
            continue;
        }
        const SwordSkill* skill = player.SkillAt(i);
        const bool target = (i == targetSlot_);

        draw::GradientRectV(rect, palette::kPanelLight.Scaled(target ? 1.1f : 0.8f),
                            palette::kPanelDark, 235, 10);
        draw::StrokeRect(rect, target ? palette::kAccent : palette::kBorder.Scaled(0.7f),
                         target ? 3.0f : 1.0f, 255);

        // スロット番号（戦闘中のキーと対応）
        const Rect keyTag = Rect::FromXYWH(rect.left + 8.0f, rect.top + 8.0f, 30.0f, 26.0f);
        draw::FillRect(keyTag, palette::kBlack, 190);
        draw::Text(FontSize::Tiny, keyTag.CenterX(), keyTag.top + 4.0f, palette::kText,
                   str::Format("%d", i + 1), draw::TextAlign::Center);

        if (skill) {
            DrawWeaponIcon(Rect(rect.right - 70.0f, rect.top + 8.0f, rect.right - 10.0f, rect.top + 68.0f),
                           skill->weapon, skill->effectColor);
            draw::Text(FontSize::Small, rect.left + 46.0f, rect.top + 10.0f, palette::kText, skill->name);
            draw::Text(FontSize::Tiny, rect.left + 16.0f, rect.bottom - 30.0f, palette::kTextDim,
                       str::Format("MP %d   CD %.1fs", static_cast<int>(skill->mpCost), skill->cooldown));
        } else {
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 12.0f, palette::kTextDisabled,
                       "空き", draw::TextAlign::Center);
        }
    }
}

} // namespace ui
} // namespace ecl
