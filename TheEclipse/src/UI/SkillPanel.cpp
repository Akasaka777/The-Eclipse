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

// --- ツリーの寸法 -------------------------------------------------------------
//   3 段のノードがツリー欄に収まるよう、縦方向は余裕を持たせた値にしている。
constexpr float kNodeWidth = 280.0f;
constexpr float kNodeHeight = 96.0f;
constexpr float kColumnGap = 360.0f;
constexpr float kTierGap = 121.0f;
constexpr float kTreeHeaderHeight = 34.0f;

const char* ColumnName(int column)
{
    return (column == 0) ? "火力系統" : "機動・範囲系統";
}

// 指定幅に収まるフォントサイズを選ぶ（長いスキル名がはみ出さないように）
FontSize FitFont(const std::string& text, float width)
{
    if (static_cast<float>(draw::TextWidth(FontSize::Small, text)) <= width) return FontSize::Small;
    return FontSize::Tiny;
}

} // namespace

SkillPanel::SkillPanel()
{
    Layout();
}

void SkillPanel::Layout()
{
    window_ = Rect::FromXYWH(200.0f, 100.0f, 1520.0f, 880.0f);

    // 見出し欄（高さ 54）の中に SP 表示を置く
    spTag_ = Rect::FromXYWH(window_.right - 280.0f, window_.top + 8.0f, 240.0f, 38.0f);

    // --- 武器種の切り替えタブ --------------------------------------------------
    weaponButtons_.clear();
    const float tabWidth = 220.0f;
    const float tabGap = 14.0f;
    const float tabTop = window_.top + 66.0f;
    for (int i = 0; i < static_cast<int>(WeaponType::Count); ++i) {
        const float x = window_.left + 40.0f + (tabWidth + tabGap) * static_cast<float>(i);
        weaponButtons_.push_back(Button(Rect::FromXYWH(x, tabTop, tabWidth, 46.0f),
                                        WeaponTypeName(static_cast<WeaponType>(i)), FontSize::Small));
    }

    // ユニークスキルのタブ（武器タブの右端に置く。ラベルは習得内容に合わせて変わる）
    uniqueTabButton_ = Button(Rect::FromXYWH(window_.right - 250.0f, tabTop, 210.0f, 46.0f),
                              "ユニーク", FontSize::Small);
    uniqueTabButton_.SetAccent(palette::kExp);

    // --- 装備スロット（下段）--------------------------------------------------
    const float slotWidth = 280.0f;
    const float slotGap = 20.0f;
    const float slotHeight = 116.0f;
    const float slotsWidth = slotWidth * kSkillSlotCount + slotGap * (kSkillSlotCount - 1);
    const float slotStart = window_.CenterX() - slotsWidth * 0.5f;
    const float slotTop = window_.bottom - 226.0f;
    for (int i = 0; i < kSkillSlotCount; ++i) {
        slotRects_[i] = Rect::FromXYWH(slotStart + (slotWidth + slotGap) * static_cast<float>(i),
                                       slotTop, slotWidth, slotHeight);
    }

    // --- ツリー欄 / 詳細欄 ----------------------------------------------------
    //   スロットの見出し（slotTop - 30）と重ならない位置で切る。
    const float bodyTop = tabTop + 66.0f;
    const float bodyBottom = slotTop - 50.0f;
    treeArea_ = Rect(window_.left + 40.0f, bodyTop, window_.left + 700.0f, bodyBottom);
    detailArea_ = Rect(window_.left + 730.0f, bodyTop, window_.right - 40.0f, bodyBottom);

    // --- 操作ボタン（詳細欄の下端に横並び）--------------------------------------
    const float buttonTop = detailArea_.bottom - 76.0f;
    unlockButton_ = Button(Rect::FromXYWH(detailArea_.left + 20.0f, buttonTop, 220.0f, 56.0f),
                           "解放する");
    unlockButton_.SetAccent(palette::kExp);
    equipButton_ = Button(Rect::FromXYWH(detailArea_.left + 260.0f, buttonTop, 220.0f, 56.0f),
                          "スロットへ装備");
    unequipButton_ = Button(Rect::FromXYWH(detailArea_.left + 500.0f, buttonTop, 180.0f, 56.0f),
                            "外す");
    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 86.0f,
                                         160.0f, 58.0f), "閉じる");
}

Rect SkillPanel::NodeRect(int column, int tier) const
{
    const float cx = treeArea_.left + 150.0f + kColumnGap * static_cast<float>(column);
    const float cy = treeArea_.top + kTreeHeaderHeight + kNodeHeight * 0.5f
                   + kTierGap * static_cast<float>(tier - 1);
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

    // タブ名は表示できるユニークスキルの名前にする
    const UniqueSkillType type = VisibleUniqueType(context);
    if (type != UniqueSkillType::None) uniqueTabButton_.SetLabel(UniqueSkillName(type));
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

bool SkillPanel::CanEquipSelected(const GameContext& context, std::string& outReason) const
{
    outReason.clear();

    const PlayerData& player = context.player;
    const SwordSkill* skill = SkillDatabase::Instance().Find(selectedSkillId_);
    if (!skill) return false;
    if (!player.IsSkillUnlocked(skill->id)) {
        outReason = "まだ解放していません";
        return false;
    }
    if (skill->weapon != player.CurrentWeaponType()) {
        outReason = str::Format("%s を装備すると使えます", WeaponTypeName(skill->weapon));
        return false;
    }
    if (skill->IsUnique() && skill->requiredUnique != player.UniqueSkill()) {
        outReason = "ユニークスキルの習得が必要です";
        return false;
    }
    // 通常スキルと専用スキルは混在できない
    if (!player.CanEquipSkill(skill->id)) {
        outReason = skill->IsUnique() ? "通常のソードスキルを外してください"
                                      : "ユニークスキルのスキルを外してください";
        return false;
    }
    return true;
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
    const UniqueSkillType uniqueType = VisibleUniqueType(context);
    if (uniqueType != UniqueSkillType::None) {
        // タブ名は習得（または解放可能）になったユニークスキルの名前
        uniqueTabButton_.SetLabel(UniqueSkillName(uniqueType));
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
                SkillDatabase::Instance().ForUnique(uniqueType);
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (NodeRect(0, static_cast<int>(i) + 1).Contains(mouseX, mouseY)) {
                    selectedSkillId_ = nodes[i]->id;
                }
            }
        } else {
            for (int column = 0; column < kSkillTreeColumns; ++column) {
                for (int tier = 1; tier <= kSkillTreeTiers; ++tier) {
                    const SwordSkill* node =
                        SkillDatabase::Instance().NodeAt(viewWeapon_, column, tier);
                    if (!node) continue;
                    if (NodeRect(column, tier).Contains(mouseX, mouseY)) {
                        selectedSkillId_ = node->id;
                    }
                }
            }
        }
        // --- スロット選択 ----------------------------------------------------
        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (slotRects_[i].Contains(mouseX, mouseY)) targetSlot_ = i;
        }
    }

    const SwordSkill* selected = SkillDatabase::Instance().Find(selectedSkillId_);
    const bool canUnlock = selected && player.CanUnlockSkill(selected->id);
    std::string equipReason;
    const bool equippable = CanEquipSelected(context, equipReason);

    // 専用スキルは 4 番目の枠に入れられないので、装備先を詰める
    const int targetLimit = (selected && selected->IsUnique()) ? kUniqueSkillSlotCount
                                                              : kSkillSlotCount;
    if (targetSlot_ >= targetLimit) targetSlot_ = 0;

    unlockButton_.SetEnabled(canUnlock);
    equipButton_.SetEnabled(equippable);
    unequipButton_.SetEnabled(player.SkillAt(targetSlot_) != nullptr);

    // --- 解放 ---------------------------------------------------------------
    if (unlockButton_.Update(input, dt) && canUnlock) {
        // ユニークスキル専用の最初のスキルを解放すると、その場で習得する
        const bool acquiresUnique = selected->IsUnique()
                                 && selected->requiredUnique != player.UniqueSkill();
        const UniqueSkillType acquired = selected->requiredUnique;
        if (player.UnlockSkill(selected->id)) {
            if (acquiresUnique) {
                message_ = str::Format("ユニークスキル「%s」を習得しました",
                                       UniqueSkillName(acquired));
                messageTimer_ = 3.4f;
            } else {
                message_ = str::Format("「%s」を解放しました", selected->name.c_str());
                messageTimer_ = 2.4f;
            }
        }
    }

    // --- 装備 / 解除 ---------------------------------------------------------
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
    draw::FillRect(spTag_, palette::kExp.Scaled(0.35f), 230);
    draw::StrokeRect(spTag_, palette::kExp, 2.0f, 255);
    draw::Text(FontSize::Normal, spTag_.CenterX(), spTag_.top + 6.0f, palette::kText,
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

void SkillPanel::DrawTabBadge(const Rect& tabRect, const char* label, const ColorRGB& color) const
{
    // タブの右上に収まる小さなバッジ（文字と重ならないよう右端に寄せる）
    const Rect badge(tabRect.right - 72.0f, tabRect.top + 4.0f,
                     tabRect.right - 6.0f, tabRect.top + 24.0f);
    draw::FillRect(badge, color.Scaled(0.35f), 235);
    draw::StrokeRect(badge, color, 1.0f, 255);
    draw::Text(FontSize::Tiny, badge.CenterX(), badge.top + 2.0f, palette::kText, label,
               draw::TextAlign::Center);
}

void SkillPanel::DrawWeaponTabs(const GameContext& context) const
{
    const PlayerData& player = context.player;

    // ユニークスキルは解放条件を満たすまで表示しない
    const UniqueSkillType uniqueType = VisibleUniqueType(context);
    if (uniqueType != UniqueSkillType::None) {
        uniqueTabButton_.Draw();
        const Rect rect = uniqueTabButton_.GetRect();

        if (!player.HasUniqueSkill()) {
            // 解放条件は満たしたがまだ習得していない
            const float pulse = 0.7f + 0.3f * std::sin(time_ * 4.0f);
            draw::StrokeRect(rect.Expanded(3.0f), palette::kExp, 2.0f,
                             static_cast<int>(200.0f * pulse));
            DrawTabBadge(rect, "解放可能", palette::kExp);
        } else if (uniqueType == UniqueSkillType::DualWield
                   && player.GetInventory().IsDualWielding()) {
            // 両手に片手剣を持っている＝二刀流が働いている状態
            DrawTabBadge(rect, "装備中", palette::kAccent);
        }
    }

    for (int i = 0; i < static_cast<int>(weaponButtons_.size()); ++i) {
        weaponButtons_[static_cast<size_t>(i)].Draw();

        // 現在装備中の武器種に印を付ける
        if (static_cast<WeaponType>(i) == player.CurrentWeaponType()) {
            DrawTabBadge(weaponButtons_[static_cast<size_t>(i)].GetRect(), "装備中",
                         palette::kAccent);
        }
    }
}

void SkillPanel::DrawNode(const Rect& rect, const SwordSkill& skill, const PlayerData& player,
                          const std::string& status, const ColorRGB& statusColor) const
{
    const bool unlocked = player.IsSkillUnlocked(skill.id);
    const bool reachable = player.IsSkillReachable(skill.id);
    const bool canUnlock = player.CanUnlockSkill(skill.id);
    const bool selected = (skill.id == selectedSkillId_);
    const bool equipped = player.SkillSlotOf(skill.id) >= 0;

    ColorRGB fill = palette::kPanelDark;
    if (unlocked) fill = ColorRGB::Lerp(palette::kPanelLight, skill.effectColor.Scaled(0.5f), 0.55f);
    else if (canUnlock) fill = palette::kPanelLight;
    draw::GradientRectV(rect, fill.Scaled(1.15f), fill.Scaled(0.75f), 235, 10);

    ColorRGB border = palette::kTextDisabled;
    if (equipped) border = palette::kAccent;
    else if (unlocked) border = skill.effectColor;
    else if (canUnlock) border = palette::kExp;
    draw::StrokeRect(rect, border, selected ? 4.0f : 2.0f, 255);

    if (selected) {
        const float pulse = 0.5f + 0.5f * std::sin(time_ * 5.0f);
        draw::StrokeRect(rect.Expanded(4.0f), palette::kWhite, 1.0f,
                         static_cast<int>(150.0f * pulse));
    }

    DrawWeaponIcon(Rect(rect.left + 8.0f, rect.top + 8.0f, rect.left + 64.0f, rect.bottom - 8.0f),
                   skill.weapon, unlocked ? skill.effectColor : palette::kTextDisabled);

    const ColorRGB textColor = unlocked ? palette::kText
                             : (reachable ? palette::kTextDim : palette::kTextDisabled);
    const float nameWidth = rect.right - 8.0f - (rect.left + 72.0f);
    draw::Text(FitFont(skill.name, nameWidth), rect.left + 72.0f, rect.top + 8.0f, textColor,
               skill.name);
    draw::Text(FontSize::Tiny, rect.left + 72.0f, rect.top + 34.0f, palette::kTextDim,
               str::Format("MP %d  CD %.1fs", static_cast<int>(skill.mpCost), skill.cooldown));
    draw::Text(FontSize::Tiny, rect.left + 72.0f, rect.bottom - 24.0f, statusColor, status);
}

void SkillPanel::DrawTree(const GameContext& context) const
{
    const PlayerData& player = context.player;
    const SkillDatabase& database = SkillDatabase::Instance();

    draw::FillRect(treeArea_.Expanded(8.0f), palette::kPanelDark, 190);
    draw::StrokeRect(treeArea_.Expanded(8.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    // 系統名（ノードの上に独立した見出し欄を確保している）
    for (int column = 0; column < kSkillTreeColumns; ++column) {
        const Rect head = NodeRect(column, 1);
        draw::Text(FontSize::Tiny, head.CenterX(), treeArea_.top + 6.0f, palette::kTextDim,
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
            const int  equippedSlot = player.SkillSlotOf(node->id);

            // 前提スキルとの接続線
            if (tier > 1) {
                const Rect parent = NodeRect(column, tier - 1);
                const ColorRGB lineColor = unlocked ? node->effectColor
                                         : (reachable ? palette::kBorder : palette::kTextDisabled);
                draw::Line(parent.CenterX(), parent.bottom, rect.CenterX(), rect.top,
                           lineColor, unlocked ? 4.0f : 2.0f, 220);
            }

            std::string status;
            ColorRGB statusColor = palette::kTextDisabled;
            if (equippedSlot >= 0) {
                status = str::Format("装備中  スロット %d", equippedSlot + 1);
                statusColor = palette::kAccent;
            } else if (unlocked) {
                status = "解放済み";
                statusColor = palette::kHp;
            } else if (!reachable) {
                status = "前提スキルが必要";
            } else {
                status = str::Format("解放に SP %d", node->unlockCost);
                statusColor = canUnlock ? palette::kExp : palette::kDanger;
            }
            DrawNode(rect, *node, player, status, statusColor);
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

    const bool acquired = (player.UniqueSkill() == type);
    const std::vector<const SwordSkill*> nodes = SkillDatabase::Instance().ForUnique(type);

    // 左列にノード、右列に効果と解放条件を並べる（重ならないよう列で分ける）
    draw::Text(FontSize::Tiny, NodeRect(0, 1).CenterX(), treeArea_.top + 6.0f, palette::kExp,
               "ユニークスキル", draw::TextAlign::Center);

    for (size_t i = 0; i < nodes.size(); ++i) {
        const SwordSkill* node = nodes[i];
        const Rect rect = NodeRect(0, static_cast<int>(i) + 1);

        const bool unlocked = player.IsSkillUnlocked(node->id);
        const bool reachable = player.IsSkillReachable(node->id);
        const bool canUnlock = player.CanUnlockSkill(node->id);
        const int  equippedSlot = player.SkillSlotOf(node->id);

        if (i > 0) {
            const Rect parent = NodeRect(0, static_cast<int>(i));
            draw::Line(parent.CenterX(), parent.bottom, rect.CenterX(), rect.top,
                       unlocked ? node->effectColor : palette::kTextDisabled,
                       unlocked ? 4.0f : 2.0f, 220);
        }

        std::string status;
        ColorRGB statusColor = palette::kTextDisabled;
        if (equippedSlot >= 0) {
            status = str::Format("装備中  スロット %d", equippedSlot + 1);
            statusColor = palette::kAccent;
        } else if (unlocked) {
            status = "解放済み";
            statusColor = palette::kHp;
        } else if (!reachable) {
            status = "前提スキルが必要";
        } else if (!acquired) {
            // 最初のスキルを解放した時点でユニークスキルを習得する
            status = str::Format("解放に SP %d（習得）", node->unlockCost);
            statusColor = canUnlock ? palette::kExp : palette::kDanger;
        } else {
            status = str::Format("解放に SP %d", node->unlockCost);
            statusColor = canUnlock ? palette::kExp : palette::kDanger;
        }
        DrawNode(rect, *node, player, status, statusColor);
    }

    // --- 右列: 効果と解放条件（ノード列の右隣を丸ごと使う）----------------------
    const float infoLeft = treeArea_.left + 300.0f;
    float y = treeArea_.top + 6.0f;
    draw::Text(FontSize::Normal, infoLeft, y, palette::kExp, def->name);
    y += 34.0f;
    draw::Text(FontSize::Small, infoLeft, y, palette::kText, def->description);
    y += 32.0f;
    draw::Line(infoLeft, y, treeArea_.right - 8.0f, y, palette::kBorder, 1.0f, 120);
    y += 12.0f;
    for (const std::string& line : def->details) {
        draw::Text(FontSize::Tiny, infoLeft, y, palette::kTextDim, line);
        y += 24.0f;
    }
    y += 10.0f;
    if (acquired) {
        draw::Text(FontSize::Small, infoLeft, y, palette::kHp, "習得済み");
    } else {
        draw::Text(FontSize::Tiny, infoLeft, y, palette::kExp,
                   str::Format("解放条件 : %s", def->unlockCondition.c_str()));
        y += 26.0f;
        draw::Text(FontSize::Tiny, infoLeft, y, palette::kAccent,
                   str::Format("「%s」の解放で習得します", nodes.empty() ? "" : nodes[0]->name.c_str()));
    }
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

    // --- 名前と説明 -----------------------------------------------------------
    draw::Text(FontSize::Large, detailArea_.left + 24.0f, detailArea_.top + 16.0f,
               skill->effectColor, skill->name);
    draw::Text(FontSize::Small, detailArea_.left + 24.0f, detailArea_.top + 62.0f,
               palette::kTextDim, skill->description);
    draw::Line(detailArea_.left + 24.0f, detailArea_.top + 92.0f,
               detailArea_.right - 24.0f, detailArea_.top + 92.0f, palette::kBorder, 1.0f, 120);

    // --- 性能（2 列 3 行に分けてボタンと重ならないようにする）----------------------
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
    const float rowTop = detailArea_.top + 104.0f;
    for (int i = 0; i < 6; ++i) {
        const float x = detailArea_.left + ((i < 3) ? 24.0f : 390.0f);
        const float y = rowTop + 32.0f * static_cast<float>(i % 3);
        draw::Text(FontSize::Small, x, y, palette::kTextDim, rows[i].label);
        draw::Text(FontSize::Small, x + 330.0f, y, palette::kText, rows[i].value,
                   draw::TextAlign::Right);
    }

    float y = rowTop + 112.0f;
    draw::Line(detailArea_.left + 24.0f, y, detailArea_.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 14.0f;

    // --- 解放・装備の状態 ------------------------------------------------------
    if (player.IsSkillUnlocked(skill->id)) {
        const int slot = player.SkillSlotOf(skill->id);
        if (slot >= 0) {
            draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y, palette::kAccent,
                       str::Format("スロット %d に装備中", slot + 1));
        } else {
            draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y, palette::kHp, "解放済み");
        }
    } else if (!player.IsSkillReachable(skill->id)) {
        const SwordSkill* parent = SkillDatabase::Instance().Find(skill->requiredSkillId);
        draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y, palette::kDanger,
                   str::Format("前提スキル : %s", parent ? parent->name.c_str() : "?"));
    } else {
        const bool enough = player.SkillPoints() >= skill->unlockCost;
        draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y,
                   enough ? palette::kExp : palette::kDanger,
                   str::Format("解放に必要な SP : %d", skill->unlockCost));
    }
    y += 30.0f;

    // 装備できない場合の理由を 1 行で出す
    std::string reason;
    if (!CanEquipSelected(context, reason) && !reason.empty()) {
        draw::Text(FontSize::Tiny, detailArea_.left + 24.0f, y, palette::kTextDim,
                   str::Format("※ %s", reason.c_str()));
    }

    unlockButton_.Draw();
    equipButton_.Draw();
    unequipButton_.Draw();
}

void SkillPanel::DrawSlots(const GameContext& context) const
{
    const PlayerData& player = context.player;

    const int limit = player.SkillSlotLimit();
    draw::Text(FontSize::Small, slotRects_[0].left, slotRects_[0].top - 32.0f, palette::kAccent,
               str::Format("装備スキル（最大 %d つ / クリックで装備先を選択）", limit));
    if (player.HasUniqueSkillEquipped()) {
        draw::Text(FontSize::Tiny, slotRects_[kSkillSlotCount - 1].right,
                   slotRects_[0].top - 28.0f, palette::kExp,
                   "ユニークスキルのスキル装備中のため 1 枠減少", draw::TextAlign::Right);
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
            // 武器アイコンは右下に置き、名前の欄を広く取る
            DrawWeaponIcon(Rect(rect.right - 58.0f, rect.bottom - 58.0f,
                                rect.right - 8.0f, rect.bottom - 8.0f),
                           skill->weapon, skill->effectColor);
            const float nameWidth = rect.right - 12.0f - (rect.left + 46.0f);
            draw::Text(FitFont(skill->name, nameWidth), rect.left + 46.0f, rect.top + 10.0f,
                       palette::kText, skill->name);
            if (skill->IsUnique()) {
                draw::Text(FontSize::Tiny, rect.left + 46.0f, rect.top + 34.0f, palette::kExp,
                           "ユニークスキル");
            }
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
