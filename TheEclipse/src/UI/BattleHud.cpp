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

// アイテムスロット（左下。スキルアイコンと同じ大きさの正方形を前後に重ねる）
constexpr float kItemCardLeft = 40.0f;
constexpr float kItemBackOffset = 26.0f;   // 奥のカードを右上へずらす量

// ボス HP ゲージ（ボスの頭上の右上）
constexpr float kBossGaugeWidth = 400.0f;
constexpr float kBossBarHeight = 14.0f;
constexpr float kBossBarGap = 6.0f;
constexpr float kBossGaugeOffsetX = 36.0f;   // 頭から右へ
constexpr float kBossGaugeOffsetY = 24.0f;   // 頭から上へ
constexpr float kBossGaugeMargin = 16.0f;    // 画面端からの余白

} // namespace

BattleHud::BattleHud()
{
    // 右下にスキルアイコンを 4 つ並べる
    for (int i = 0; i < kSkillSlotCount; ++i) {
        const float x = kScreenW - 40.0f - kSkillIconSize * static_cast<float>(4 - i)
                      - 14.0f * static_cast<float>(3 - i);
        skillRects_[i] = Rect::FromXYWH(x, kSkillBarBottom - kSkillIconSize, kSkillIconSize, kSkillIconSize);
    }

    // アイテムスロット：手前のカードはスキルバーと底をそろえ、奥のカードを右上へずらす
    itemFront_ = Rect::FromXYWH(kItemCardLeft, kSkillBarBottom - kSkillIconSize, kSkillIconSize,
                                kSkillIconSize);
    itemBack_ = Rect::FromXYWH(itemFront_.left + kItemBackOffset, itemFront_.top - kItemBackOffset,
                               kSkillIconSize, kSkillIconSize);
}

Rect BattleHud::ItemAreaRect() const
{
    return Rect(itemFront_.left, itemBack_.top, itemBack_.right, itemFront_.bottom);
}

float BattleHud::BossBarFill(float ratio, int index, int bars)
{
    // index 本目が受け持つ範囲：上から順に [1 - (index+1)/N, 1 - index/N]
    const float share = 1.0f / static_cast<float>(math::MaxI(1, bars));
    const float low = 1.0f - share * static_cast<float>(index + 1);
    return math::Clamp((ratio - low) / share, 0.0f, 1.0f);
}

int BattleHud::BossBarCount(const Boss& boss)
{
    const BossDef* def = boss.Def();
    return (def && def->hpBarCount > 0) ? def->hpBarCount : 3;
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

    // --- アイテムスロット ------------------------------------------------------
    auto swapSlot = [this]() {
        itemSlot_ = (itemSlot_ == QuickSlot::Recovery) ? QuickSlot::Buff : QuickSlot::Recovery;
    };
    if (input.Pressed(GameAction::ItemSwitch)) swapSlot();
    if (input.Pressed(GameAction::ItemUse)) itemUseRequested_ = true;

    if (input.MouseClicked(MouseButton::Left) && ItemAreaRect().Contains(mouseX, mouseY)) {
        // 手前のカードは使う、見えている奥のカードは前後を入れ替える
        if (itemFront_.Contains(mouseX, mouseY)) {
            itemUseRequested_ = true;
            itemClickConsumed_ = true;
        } else if (itemBack_.Contains(mouseX, mouseY)) {
            swapSlot();
            itemClickConsumed_ = true;
        }
    }

    const float target = (itemSlot_ == QuickSlot::Buff) ? 1.0f : 0.0f;
    itemSlide_ = math::Approach(itemSlide_, target, dt * 7.0f);
}

void BattleHud::Draw(const Player& player, const PlayerData& data, const Boss* boss,
                     const HudInfo& info) const
{
    DrawPlayerStatus(player, data);
    if (boss && boss->Def()) DrawBossStatus(*boss, info);
    DrawSkillBar(player, data);
    DrawItemSlots(player, data);
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

void BattleHud::DrawBossStatus(const Boss& boss, const HudInfo& info) const
{
    const BossDef* def = boss.Def();
    const int bars = BossBarCount(boss);

    // 枠の大きさ：肩書き・名前・ゲージ N 本・HP の数値
    const float height = 12.0f + 18.0f + 26.0f + 8.0f
                       + (kBossBarHeight + kBossBarGap) * static_cast<float>(bars) + 22.0f;

    // ボスの頭の右上に置く。画面からはみ出す場合は内側へ寄せる
    const Vec2 head = info.hasBossHead ? info.bossHead : Vec2(kScreenW * 0.5f, 400.0f);
    float left = head.x + kBossGaugeOffsetX;
    float bottom = head.y - kBossGaugeOffsetY;
    left = math::Clamp(left, kBossGaugeMargin, kScreenW - kBossGaugeMargin - kBossGaugeWidth);
    // 左上のプレイヤー情報（〜x 648, y 152）とフロア情報（〜x 448, y 240）の下へ逃がす
    float minTop = 24.0f;
    if (left < 660.0f) minTop = 166.0f;
    if (left < 460.0f) minTop = 252.0f;
    bottom = math::Clamp(bottom, minTop + height, 860.0f);
    const Rect panel(left, bottom - height, left + kBossGaugeWidth, bottom);
    bossGauge_ = panel;

    draw::ChamferRect(panel, 10.0f, palette::kPanelDark, 205);
    draw::StrokeRect(panel, palette::kBossHp.Scaled(0.8f), 2.0f, 210);

    float y = panel.top + 8.0f;
    draw::Text(FontSize::Tiny, panel.left + 14.0f, y, palette::kTextDim, def->title);
    // フェーズ表示
    for (int i = 0; i < 3; ++i) {
        const float x = panel.right - 20.0f - static_cast<float>(2 - i) * 20.0f;
        const bool active = (i < boss.Phase());
        draw::Circle(x, y + 8.0f, 6.0f, active ? palette::kBossHp : palette::kTextDisabled,
                     true, 1.0f, 255);
    }
    y += 20.0f;
    draw::TextShadow(FontSize::Small, panel.left + 14.0f, y, palette::kText, def->name);
    y += 30.0f;

    // --- HP ゲージ（上の 1 本から順に減り、下へ連なる）---------------------------
    const float ratio = boss.HpRatio();
    const ColorRGB color = palette::HpColor(ratio);
    const float share = 1.0f / static_cast<float>(bars);
    for (int i = 0; i < bars; ++i) {
        const float fill = BossBarFill(ratio, i, bars);
        const float delay = BossBarFill(bossHpDelay_, i, bars);

        const Rect bar(panel.left + 14.0f, y, panel.right - 14.0f, y + kBossBarHeight);
        draw::Bar(bar, fill, color, palette::kBlack, delay, ColorRGB(255, 190, 120));
        draw::StrokeRect(bar, (fill > 0.0f) ? palette::kBorder.Scaled(0.8f) : palette::kTextDisabled,
                         1.0f, 170);
        y += kBossBarHeight + kBossBarGap;
    }

    draw::Text(FontSize::Tiny, panel.right - 14.0f, y, palette::kTextDim,
               str::Format("%d / %d", static_cast<int>(boss.hp), static_cast<int>(boss.maxHp)),
               draw::TextAlign::Right);
    // 残りのゲージ本数
    const int remaining = (ratio <= 0.0f) ? 0
                        : math::ClampInt(static_cast<int>(std::ceil(ratio / share - 0.0001f)), 1, bars);
    draw::Text(FontSize::Tiny, panel.left + 14.0f, y, palette::kBossHp,
               str::Format("×%d", remaining));
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

void BattleHud::DrawItemSlots(const Player& player, const PlayerData& data) const
{
    // 入れ替えの途中は 2 枚が前後の位置を行き来する。
    //   回復カードは itemSlide_ = 0 で手前、1 で奥。バフカードはその逆。
    auto lerpRect = [](const Rect& a, const Rect& b, float t) {
        return Rect(math::Lerp(a.left, b.left, t), math::Lerp(a.top, b.top, t),
                    math::Lerp(a.right, b.right, t), math::Lerp(a.bottom, b.bottom, t));
    };
    // 奥へ回るカードは一度左上へ浮かせて、すれ違うように見せる
    const float lift = std::sin(itemSlide_ * 3.14159265f) * 22.0f;
    Rect recovery = lerpRect(itemFront_, itemBack_, itemSlide_);
    Rect buff = lerpRect(itemBack_, itemFront_, itemSlide_);
    if (itemSlot_ == QuickSlot::Buff) { recovery.top -= lift; recovery.bottom -= lift; }
    else                               { buff.top -= lift; buff.bottom -= lift; }

    // 手前に近い方を後から描く
    const bool recoveryFront = itemSlide_ < 0.5f;
    if (recoveryFront) {
        DrawItemCard(buff, QuickSlot::Buff, false, player, data);
        DrawItemCard(recovery, QuickSlot::Recovery, true, player, data);
    } else {
        DrawItemCard(recovery, QuickSlot::Recovery, false, player, data);
        DrawItemCard(buff, QuickSlot::Buff, true, player, data);
    }

    // 選んでいる枠と操作キー
    const Rect area = ItemAreaRect();
    draw::Text(FontSize::Tiny, area.left, area.top - 24.0f, palette::kTextDim,
               str::Format("%sアイテム   %s : 切替  %s : 使用", QuickSlotName(itemSlot_),
                           Input::Instance().ActionKeyName(GameAction::ItemSwitch),
                           Input::Instance().ActionKeyName(GameAction::ItemUse)));

    // 通知（スロットの上）
    if (itemMessageTimer_ > 0.0f) {
        const int alpha = static_cast<int>(math::Clamp(itemMessageTimer_ / 0.4f, 0.0f, 1.0f) * 255.0f);
        draw::TextAlpha(FontSize::Small, area.left, area.top - 54.0f, palette::kDanger,
                        itemMessage_, alpha);
    }
}

void BattleHud::DrawItemCard(const Rect& rect, QuickSlot slot, bool front, const Player& player,
                             const PlayerData& data) const
{
    const Inventory& inventory = data.GetInventory();
    const ConsumableDef* item = ConsumableDatabase::Instance().Find(inventory.QuickItem(slot));
    const int count = item ? inventory.ItemCount(item->id) : 0;
    const bool usable = item && count > 0 && player.ItemCooldown() <= 0.0f;
    const ColorRGB slotColor = (slot == QuickSlot::Buff) ? ColorRGB(255, 170, 60)
                                                         : ColorRGB(110, 230, 130);
    const ColorRGB itemColor = item ? item->color : slotColor;

    draw::ChamferRect(rect, 10.0f, palette::kPanelDark, front ? 235 : 215);
    draw::GradientRectV(rect.Expanded(-4.0f), itemColor.Scaled(front && usable ? 0.42f : 0.16f),
                        palette::kPanelDark, front ? 235 : 200, 10);

    if (!front) {
        // 奥のカード：見えている右上の帯に種類と切り替えキーだけ出す
        draw::StrokeRect(rect, slotColor.Scaled(0.6f), 2.0f, 200);
        const ConsumableDef* buff = (slot == QuickSlot::Buff) ? player.ItemBuff() : nullptr;
        draw::Text(FontSize::Tiny, rect.right - 6.0f, rect.top + 4.0f,
                   buff ? buff->color : palette::kTextDim,
                   buff ? str::Format("%s %.0fs", QuickSlotName(slot), player.ItemBuffRemain())
                        : std::string(QuickSlotName(slot)),
                   draw::TextAlign::Right);
        return;
    }

    if (!item) {
        draw::StrokeRect(rect, palette::kTextDisabled, 2.0f, 200);
        draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 20.0f, palette::kTextDisabled,
                   "---", draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, rect.CenterX(), rect.bottom - 22.0f, palette::kTextDisabled,
                   "未装備", draw::TextAlign::Center);
    } else {
        // 左上の使用キーと右上の個数に掛からない大きさ
        DrawConsumableIcon(Rect::FromCenter(rect.CenterX(), rect.CenterY() + 2.0f, 48.0f, 48.0f),
                           *item, count <= 0);

        // 使った直後の待ち時間（スキルと同じく下から満ちる）
        if (player.ItemCooldown() > 0.0f) {
            Rect cover = rect.Expanded(-4.0f);
            cover.top = cover.bottom - cover.Height() * (player.ItemCooldown() / kItemUseCooldown);
            draw::FillRect(cover, palette::kBlack, 150);
        }

        draw::Text(FontSize::Tiny, rect.CenterX(), rect.bottom - 22.0f,
                   count > 0 ? palette::kText : palette::kTextDisabled, item->name,
                   draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, rect.right - 6.0f, rect.top + 4.0f,
                   count > 0 ? palette::kAccentWarm : palette::kDanger,
                   str::Format("×%d", count), draw::TextAlign::Right);
        draw::StrokeRect(rect, usable ? item->color : palette::kTextDisabled, usable ? 3.0f : 2.0f, 255);
        if (usable) {
            const float pulse = 0.5f + 0.5f * std::sin(time_ * 4.0f);
            draw::StrokeRect(rect.Expanded(3.0f), item->color, 1.0f, static_cast<int>(120.0f * pulse));
        }
    }

    // 効いているバフの残り時間（バフカードの上端に細い帯）
    if (slot == QuickSlot::Buff) {
        if (const ConsumableDef* buff = player.ItemBuff()) {
            const Rect bar(rect.left + 6.0f, rect.top + 28.0f, rect.right - 6.0f, rect.top + 33.0f);
            draw::Bar(bar, player.ItemBuffRatio(), buff->color, palette::kPanel);
        }
    }

    // 使用キー（スキルの番号と同じ位置）
    const Rect keyTag = Rect::FromXYWH(rect.left + 4.0f, rect.top + 4.0f, 24.0f, 22.0f);
    draw::FillRect(keyTag, palette::kBlack, 190);
    draw::Text(FontSize::Tiny, keyTag.CenterX(), keyTag.top + 2.0f, palette::kText,
               Input::Instance().ActionKeyName(GameAction::ItemUse), draw::TextAlign::Center);
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
