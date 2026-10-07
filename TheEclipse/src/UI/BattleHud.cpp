#include "UI/BattleHud.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Graphics/CharacterArt.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {
namespace ui {

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kSkillIconSize = 104.0f;
constexpr float kSkillBarBottom = 1030.0f;

// アイテムスライダー（スキルバーの左に置く）
constexpr float kItemPanelWidth = 330.0f;
constexpr float kItemPanelHeight = 128.0f;
constexpr float kItemPanelGap = 30.0f;
constexpr float kItemTabHeight = 36.0f;

} // namespace

BattleHud::BattleHud()
{
    // 右下にスキルアイコンを 4 つ並べる
    for (int i = 0; i < kSkillSlotCount; ++i) {
        const float x = kScreenW - 40.0f - kSkillIconSize * static_cast<float>(4 - i)
                      - 14.0f * static_cast<float>(3 - i);
        skillRects_[i] = Rect::FromXYWH(x, kSkillBarBottom - kSkillIconSize, kSkillIconSize, kSkillIconSize);
    }

    // アイテムスライダー：上段が 回復 / バフ の見出し、下段がアイテム本体
    const float right = skillRects_[0].left - kItemPanelGap;
    itemPanel_ = Rect(right - kItemPanelWidth, kSkillBarBottom - kItemPanelHeight, right,
                      kSkillBarBottom);
    const float half = itemPanel_.Width() * 0.5f;
    for (int i = 0; i < kQuickSlotCount; ++i) {
        const float left = itemPanel_.left + half * static_cast<float>(i);
        itemTabs_[i] = Rect(left, itemPanel_.top, left + half, itemPanel_.top + kItemTabHeight);
    }
    itemBody_ = Rect(itemPanel_.left, itemPanel_.top + kItemTabHeight, itemPanel_.right,
                     itemPanel_.bottom);
}

const Rect& BattleHud::ItemTabRect(QuickSlot slot) const
{
    return itemTabs_[math::ClampInt(static_cast<int>(slot), 0, kQuickSlotCount - 1)];
}

void BattleHud::ShowItemMessage(const std::string& message)
{
    itemMessage_ = message;
    itemMessageTimer_ = 1.6f;
}

void BattleHud::Reset()
{
    hpDelay_ = 1.0f;
    bossHpDelay_ = 1.0f;
    clickedSkill_ = -1;
    time_ = 0.0f;
    itemUseRequested_ = false;
    itemClickConsumed_ = false;
    itemMessage_.clear();
    itemMessageTimer_ = 0.0f;
    // 選んでいた枠は持ち越す（毎回 回復 に戻ると使いにくいので）
    itemSlide_ = (itemSlot_ == QuickSlot::Buff) ? 1.0f : 0.0f;
}

void BattleHud::Update(float dt, const Player& player, const Boss* boss, const Input& input)
{
    time_ += dt;
    clickedSkill_ = -1;
    itemUseRequested_ = false;
    itemClickConsumed_ = false;
    itemMessageTimer_ = math::MaxF(0.0f, itemMessageTimer_ - dt);

    // HP バーの減少残像
    hpDelay_ = math::Approach(hpDelay_, player.HpRatio(), dt * 0.55f);
    if (hpDelay_ < player.HpRatio()) hpDelay_ = player.HpRatio();

    if (boss) {
        bossHpDelay_ = math::Approach(bossHpDelay_, boss->HpRatio(), dt * 0.4f);
        if (bossHpDelay_ < boss->HpRatio()) bossHpDelay_ = boss->HpRatio();
    }

    // スキルアイコンのクリック
    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());
    if (input.MouseClicked(MouseButton::Left)) {
        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (player.Skill(i) == nullptr) continue;
            if (skillRects_[i].Contains(mouseX, mouseY)) {
                clickedSkill_ = i;
                break;
            }
        }
    }

    // --- アイテムスライダー ----------------------------------------------------
    if (input.Pressed(GameAction::ItemSwitch)) {
        itemSlot_ = (itemSlot_ == QuickSlot::Recovery) ? QuickSlot::Buff : QuickSlot::Recovery;
    }
    if (input.Pressed(GameAction::ItemUse)) itemUseRequested_ = true;

    if (input.MouseClicked(MouseButton::Left) && itemPanel_.Contains(mouseX, mouseY)) {
        itemClickConsumed_ = true;
        bool onTab = false;
        for (int i = 0; i < kQuickSlotCount; ++i) {
            if (!itemTabs_[i].Contains(mouseX, mouseY)) continue;
            itemSlot_ = static_cast<QuickSlot>(i);
            onTab = true;
        }
        if (!onTab && itemBody_.Contains(mouseX, mouseY)) itemUseRequested_ = true;
    }

    const float target = (itemSlot_ == QuickSlot::Buff) ? 1.0f : 0.0f;
    itemSlide_ = math::Approach(itemSlide_, target, dt * 7.0f);
}

void BattleHud::Draw(const Player& player, const PlayerData& data, const Boss* boss,
                     const HudInfo& info) const
{
    DrawPlayerStatus(player, data);
    if (boss && boss->Def()) DrawBossStatus(*boss);
    DrawSkillBar(player, data);
    DrawItemSlider(player, data);
    DrawFloorInfo(info);
    DrawCombo(info);

    if (info.showFps) {
        draw::Text(FontSize::Small, kScreenW - 30.0f, 30.0f, palette::kTextDim,
                   str::Format("FPS %.1f", GetFPS()), draw::TextAlign::Right);
    }
}

void BattleHud::DrawPlayerStatus(const Player& player, const PlayerData& data) const
{
    const Rect panel = Rect::FromXYWH(28.0f, 24.0f, 620.0f, 128.0f);

    // 背景（左上に寄せた独自デザイン）
    draw::ChamferRect(panel, 18.0f, palette::kPanel, 225);
    draw::StrokeRect(panel, palette::kBorder, 2.0f, 200);

    // --- キャラアイコン ------------------------------------------------------
    const Rect icon = Rect::FromXYWH(panel.left + 14.0f, panel.top + 14.0f, 100.0f, 100.0f);
    draw::GradientRectV(icon, palette::kPanelLight, palette::kPanelDark, 255, 10);
    draw::StrokeRect(icon, palette::kAccent, 2.0f, 255);
    {
        // アイコン内にプレイヤーの姿を縮小表示
        ActorArt art = player.art;
        const Rect body = Rect::FromXYWH(icon.left + 18.0f, icon.top + 8.0f, 64.0f, 84.0f);
        DrawActor(body, 1, player.IsDead() ? PoseKind::Dead : PoseKind::Idle,
                  std::fmod(time_ * 0.5f, 1.0f), art, nullptr, 255, 0.0f);
    }
    // レベル表示
    const Rect levelTag = Rect::FromXYWH(icon.left - 4.0f, icon.bottom - 26.0f, 54.0f, 26.0f);
    draw::FillRect(levelTag, palette::kAccent, 235);
    draw::Text(FontSize::Small, levelTag.CenterX(), levelTag.top + 2.0f, palette::kBlack,
               str::Format("Lv%d", data.Level()), draw::TextAlign::Center);

    // --- 名前 ---------------------------------------------------------------
    draw::Text(FontSize::Small, icon.right + 18.0f, panel.top + 12.0f, palette::kTextDim, data.Name());

    // --- HP -----------------------------------------------------------------
    const Rect hpBar = Rect::FromXYWH(icon.right + 18.0f, panel.top + 42.0f, 460.0f, 28.0f);
    // 残り 25% ごとに色が変わる（緑 → 黄緑 → 黄 → 赤）
    draw::SkewBar(hpBar, player.HpRatio(), palette::HpColor(player.HpRatio()), palette::kPanelDark,
                  10.0f, hpDelay_, palette::kHpLoss);
    draw::StrokeRect(hpBar, palette::kBorder.Scaled(0.8f), 1.0f, 150);
    draw::Text(FontSize::Tiny, hpBar.left + 8.0f, hpBar.top + 4.0f, palette::kText, "HP");
    draw::TextShadow(FontSize::Small, hpBar.right - 8.0f, hpBar.top + 2.0f, palette::kText,
                     str::Format("%d / %d", static_cast<int>(player.hp), static_cast<int>(player.maxHp)),
                     draw::TextAlign::Right);

    // --- MP -----------------------------------------------------------------
    const Rect mpBar = Rect::FromXYWH(icon.right + 18.0f, panel.top + 78.0f, 380.0f, 20.0f);
    draw::SkewBar(mpBar, player.MpRatio(), palette::kMp, palette::kPanelDark, 8.0f);
    draw::StrokeRect(mpBar, palette::kBorder.Scaled(0.8f), 1.0f, 150);
    draw::Text(FontSize::Tiny, mpBar.left + 8.0f, mpBar.top + 1.0f, palette::kText, "MP");
    draw::TextShadow(FontSize::Tiny, mpBar.right - 8.0f, mpBar.top + 1.0f, palette::kText,
                     str::Format("%d / %d", static_cast<int>(player.Mp()), static_cast<int>(player.MaxMp())),
                     draw::TextAlign::Right);

    // --- パリィ状態（ガード中のみ表示） ---------------------------------------
    if (player.IsGuarding() || player.IsParryActive()) {
        const Rect badge = Rect::FromXYWH(mpBar.right + 14.0f, mpBar.top - 4.0f, 96.0f, 28.0f);

        ColorRGB color = palette::kAccent;
        const char* label = "パリィ可";
        if (player.IsParryActive()) {
            color = palette::kCritical;
            label = "受付中";
        } else if (player.ParryCooldown() > 0.0f) {
            color = palette::kTextDisabled;
            label = "硬直";
        }

        draw::FillRect(badge, color.Scaled(0.35f), 220);
        draw::StrokeRect(badge, color, 2.0f, 235);
        draw::Text(FontSize::Tiny, badge.CenterX(), badge.top + 5.0f, palette::kText, label,
                   draw::TextAlign::Center);
    }
}

void BattleHud::DrawBossStatus(const Boss& boss) const
{
    const BossDef* def = boss.Def();
    const Rect panel = Rect::FromXYWH(kScreenW * 0.5f - 460.0f, 30.0f, 920.0f, 96.0f);

    draw::ChamferRect(panel, 14.0f, palette::kPanelDark, 215);
    draw::StrokeRect(panel, palette::kBossHp.Scaled(0.8f), 2.0f, 220);

    draw::Text(FontSize::Small, panel.left + 20.0f, panel.top + 8.0f, palette::kTextDim, def->title);
    draw::TextShadow(FontSize::Medium, panel.left + 20.0f, panel.top + 28.0f, palette::kText, def->name);

    // フェーズ表示
    for (int i = 0; i < 3; ++i) {
        const float x = panel.right - 30.0f - static_cast<float>(2 - i) * 26.0f;
        const bool active = (i < boss.Phase());
        draw::Circle(x, panel.top + 24.0f, 8.0f, active ? palette::kBossHp : palette::kTextDisabled,
                     true, 1.0f, 255);
    }

    const Rect bar = Rect::FromXYWH(panel.left + 20.0f, panel.bottom - 30.0f, panel.Width() - 40.0f, 20.0f);
    draw::Bar(bar, boss.HpRatio(), palette::HpColor(boss.HpRatio()), palette::kPanelDark,
              bossHpDelay_, ColorRGB(255, 190, 120));
    draw::StrokeRect(bar, palette::kBorder.Scaled(0.7f), 1.0f, 160);

    // フェーズ境界の目盛り
    for (float ratio : { 0.35f, 0.70f }) {
        const float x = bar.left + bar.Width() * ratio;
        draw::Line(x, bar.top, x, bar.bottom, palette::kBlack, 2.0f, 180);
    }

    draw::Text(FontSize::Tiny, bar.right, bar.top - 20.0f, palette::kTextDim,
               str::Format("%d / %d", static_cast<int>(boss.hp), static_cast<int>(boss.maxHp)),
               draw::TextAlign::Right);
}

void BattleHud::DrawSkillBar(const Player& player, const PlayerData& data) const
{
    const int limit = data.SkillSlotLimit();
    for (int i = 0; i < limit; ++i) {
        const Rect rect = skillRects_[i];
        const SwordSkill* skill = player.Skill(i);

        draw::ChamferRect(rect, 10.0f, palette::kPanelDark, 225);

        if (!skill) {
            draw::StrokeRect(rect, palette::kTextDisabled, 2.0f, 180);
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 10.0f, palette::kTextDisabled,
                       "---", draw::TextAlign::Center);
            continue;
        }

        const bool ready = player.CanUseSkill(i);
        const float cooldownRatio = player.SkillCooldownRatio(i);
        const ColorRGB frame = ready ? skill->effectColor : palette::kTextDisabled;

        // アイコン本体
        draw::GradientRectV(rect.Expanded(-4.0f), skill->effectColor.Scaled(ready ? 0.45f : 0.18f),
                            palette::kPanelDark, 235, 10);
        DrawWeaponIcon(Rect::FromCenter(rect.CenterX(), rect.CenterY() - 6.0f, 56.0f, 56.0f),
                       skill->weapon, ready ? skill->effectColor : palette::kTextDisabled);

        // クールダウン（下から満ちる）
        if (cooldownRatio > 0.0f) {
            Rect cover = rect.Expanded(-4.0f);
            cover.top = cover.bottom - cover.Height() * cooldownRatio;
            draw::FillRect(cover, palette::kBlack, 165);
            draw::Text(FontSize::Medium, rect.CenterX(), rect.CenterY() - 18.0f, palette::kText,
                       str::Format("%.1f", player.SkillCooldown(i)), draw::TextAlign::Center);
        } else if (!player.HasWeapon()) {
            // 素手ではソードスキルを振れない
            draw::FillRect(rect.Expanded(-4.0f), palette::kBlack, 130);
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 10.0f, palette::kDanger,
                       "素手", draw::TextAlign::Center);
        } else if (player.Mp() < skill->mpCost) {
            // MP 不足
            draw::FillRect(rect.Expanded(-4.0f), palette::kBlack, 120);
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 10.0f, palette::kDanger,
                       "MP不足", draw::TextAlign::Center);
        }

        draw::StrokeRect(rect, frame, ready ? 3.0f : 2.0f, 255);
        if (ready) {
            const float pulse = 0.5f + 0.5f * std::sin(time_ * 4.0f + static_cast<float>(i));
            draw::StrokeRect(rect.Expanded(3.0f), skill->effectColor,
                             1.0f, static_cast<int>(120.0f * pulse));
        }

        // キー番号
        const Rect keyTag = Rect::FromXYWH(rect.left + 4.0f, rect.top + 4.0f, 24.0f, 22.0f);
        draw::FillRect(keyTag, palette::kBlack, 190);
        draw::Text(FontSize::Tiny, keyTag.CenterX(), keyTag.top + 2.0f, palette::kText,
                   str::Format("%d", i + 1), draw::TextAlign::Center);

        // スキル名と消費 MP
        draw::Text(FontSize::Tiny, rect.CenterX(), rect.bottom - 22.0f, palette::kText,
                   skill->name, draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, rect.right - 6.0f, rect.top + 4.0f, palette::kMp,
                   str::Format("%d", static_cast<int>(skill->mpCost)), draw::TextAlign::Right);
    }

    // 操作ヒント
    draw::Text(FontSize::Tiny, skillRects_[0].left, skillRects_[0].top - 26.0f, palette::kTextDim,
               str::Format("1〜%d / クリックでソードスキル     ESC : メニュー", limit));
}

void BattleHud::DrawItemSlider(const Player& player, const PlayerData& data) const
{
    const Inventory& inventory = data.GetInventory();
    const Rect& panel = itemPanel_;

    draw::ChamferRect(panel, 10.0f, palette::kPanelDark, 225);
    draw::StrokeRect(panel, palette::kBorder.Scaled(0.8f), 1.0f, 200);

    // --- 見出し（回復 / バフ）と滑る帯 --------------------------------------------
    const float half = panel.Width() * 0.5f;
    const float slideLeft = panel.left + half * itemSlide_;
    const Rect highlight(slideLeft + 4.0f, panel.top + 4.0f, slideLeft + half - 4.0f,
                         panel.top + kItemTabHeight - 2.0f);
    const ColorRGB slideColor = ColorRGB::Lerp(ColorRGB(110, 230, 130), ColorRGB(255, 170, 60),
                                               itemSlide_);
    draw::FillRect(highlight, slideColor.Scaled(0.40f), 235);
    draw::Line(highlight.left, highlight.bottom, highlight.right, highlight.bottom, slideColor,
               3.0f, 255);

    const ConsumableDef* activeBuff = player.ItemBuff();
    for (int i = 0; i < kQuickSlotCount; ++i) {
        const QuickSlot slot = static_cast<QuickSlot>(i);
        const bool active = (slot == itemSlot_);
        // バフが効いている間は、バフの見出しに残り時間を添える
        if (slot == QuickSlot::Buff && activeBuff) {
            draw::Text(FontSize::Small, itemTabs_[i].CenterX(), itemTabs_[i].top + 5.0f,
                       activeBuff->color,
                       str::Format("%s %.0fs", QuickSlotName(slot), player.ItemBuffRemain()),
                       draw::TextAlign::Center);
            continue;
        }
        draw::Text(FontSize::Small, itemTabs_[i].CenterX(), itemTabs_[i].top + 5.0f,
                   active ? palette::kText : palette::kTextDim, QuickSlotName(slot),
                   draw::TextAlign::Center);
    }
    draw::Line(panel.left + 8.0f, itemBody_.top, panel.right - 8.0f, itemBody_.top,
               palette::kBorder, 1.0f, 140);

    // 操作キー（パネルの上に小さく出す）
    draw::Text(FontSize::Tiny, panel.left + 4.0f, panel.top - 26.0f, palette::kTextDim,
               str::Format("%s : 切替", Input::Instance().ActionKeyName(GameAction::ItemSwitch)));
    draw::Text(FontSize::Tiny, panel.right - 4.0f, panel.top - 26.0f, palette::kTextDim,
               str::Format("%s / クリック : 使用",
                           Input::Instance().ActionKeyName(GameAction::ItemUse)),
               draw::TextAlign::Right);

    // --- 本体 -------------------------------------------------------------------
    const Rect icon = Rect::FromXYWH(itemBody_.left + 12.0f, itemBody_.top + 8.0f, 72.0f, 72.0f);
    const float textX = icon.right + 14.0f;
    const ConsumableDef* item = ConsumableDatabase::Instance().Find(inventory.QuickItem(itemSlot_));

    if (!item) {
        draw::StrokeRect(icon, palette::kTextDisabled, 1.0f, 160);
        draw::Text(FontSize::Small, icon.CenterX(), icon.CenterY() - 12.0f, palette::kTextDisabled,
                   "---", draw::TextAlign::Center);
        draw::Text(FontSize::Small, textX, itemBody_.top + 12.0f, palette::kTextDisabled, "未装備");
        draw::Text(FontSize::Tiny, textX, itemBody_.top + 46.0f, palette::kTextDim,
                   "プレイヤー → アイテムで装備");
    } else {
        const int count = inventory.ItemCount(item->id);
        const bool empty = (count <= 0);
        DrawConsumableIcon(icon, *item, empty);
        draw::Text(FontSize::Small, textX, itemBody_.top + 10.0f,
                   empty ? palette::kTextDisabled : palette::kText, item->name);
        draw::Text(FontSize::Small, itemBody_.right - 12.0f, itemBody_.top + 10.0f,
                   empty ? palette::kDanger : palette::kAccentWarm,
                   str::Format("×%d", count), draw::TextAlign::Right);
        draw::Text(FontSize::Tiny, textX, itemBody_.top + 42.0f, palette::kTextDim,
                   item->shortEffect);

        // 使った直後の待ち時間（下から満ちる）
        if (player.ItemCooldown() > 0.0f) {
            Rect cover = icon;
            cover.top = cover.bottom - cover.Height() * (player.ItemCooldown() / kItemUseCooldown);
            draw::FillRect(cover, palette::kBlack, 150);
        }
    }

    // --- 効いているバフの残り時間（本体の下端に細い帯で出す）---------------------
    if (activeBuff) {
        const Rect bar(textX, itemBody_.bottom - 14.0f, itemBody_.right - 12.0f,
                       itemBody_.bottom - 8.0f);
        draw::Bar(bar, player.ItemBuffRatio(), activeBuff->color, palette::kPanel);
    }

    // --- 通知（パネルの上） -------------------------------------------------------
    if (itemMessageTimer_ > 0.0f) {
        const int alpha = static_cast<int>(math::Clamp(itemMessageTimer_ / 0.4f, 0.0f, 1.0f) * 255.0f);
        draw::TextAlpha(FontSize::Small, panel.CenterX(), panel.top - 58.0f, palette::kDanger,
                        itemMessage_, alpha, draw::TextAlign::Center);
    }
}

void BattleHud::DrawFloorInfo(const HudInfo& info) const
{
    const Rect panel = Rect::FromXYWH(28.0f, 166.0f, 420.0f, 74.0f);
    draw::ChamferRect(panel, 10.0f, palette::kPanelDark, 200);
    draw::StrokeRect(panel, palette::kBorder.Scaled(0.7f), 1.0f, 160);

    draw::Text(FontSize::Small, panel.left + 16.0f, panel.top + 8.0f, palette::kAccent,
               str::Format("FLOOR %d / %d", info.floorIndex, info.floorCount));
    draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 8.0f, palette::kTextDim,
               str::TimeText(info.questTime), draw::TextAlign::Right);
    draw::Text(FontSize::Small, panel.left + 16.0f, panel.top + 38.0f, palette::kText, info.floorName);

    if (info.enemiesRemaining > 0) {
        draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 38.0f, palette::kDanger,
                   str::Format("残 %d 体", info.enemiesRemaining), draw::TextAlign::Right);
    } else {
        draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 38.0f, palette::kHp,
                   "CLEAR", draw::TextAlign::Right);
    }
}

void BattleHud::DrawCombo(const HudInfo& info) const
{
    if (info.combo < 2) return;

    const float fade = math::Clamp(info.comboTimer / 0.6f, 0.0f, 1.0f);
    const int alpha = static_cast<int>(255.0f * fade);
    const float scale = 1.0f + math::Clamp(info.comboTimer - 1.2f, 0.0f, 0.4f);

    const float x = 120.0f;
    const float y = 300.0f;

    draw::TextAlpha(FontSize::Huge, x, y, palette::kAccentWarm, str::Format("%d", info.combo), alpha);
    draw::TextAlpha(FontSize::Medium, x + 96.0f * scale, y + 24.0f, palette::kText, "COMBO", alpha);
}

} // namespace ui
} // namespace ecl
