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

} // namespace

BattleHud::BattleHud()
{
    // 右下にスキルアイコンを 4 つ並べる
    for (int i = 0; i < kSkillSlotCount; ++i) {
        const float x = kScreenW - 40.0f - kSkillIconSize * static_cast<float>(4 - i)
                      - 14.0f * static_cast<float>(3 - i);
        skillRects_[i] = Rect::FromXYWH(x, kSkillBarBottom - kSkillIconSize, kSkillIconSize, kSkillIconSize);
    }
}

void BattleHud::Reset()
{
    hpDelay_ = 1.0f;
    bossHpDelay_ = 1.0f;
    clickedSkill_ = -1;
    time_ = 0.0f;
}

void BattleHud::Update(float dt, const Player& player, const Boss* boss, const Input& input)
{
    time_ += dt;
    clickedSkill_ = -1;

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
}

void BattleHud::Draw(const Player& player, const PlayerData& data, const Boss* boss,
                     const HudInfo& info) const
{
    DrawPlayerStatus(player, data);
    if (boss && boss->Def()) DrawBossStatus(*boss);
    DrawSkillBar(player, data);
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
    draw::SkewBar(hpBar, player.HpRatio(), palette::kHp, palette::kPanelDark, 10.0f, hpDelay_,
                  palette::kHpLoss);
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
    draw::Bar(bar, boss.HpRatio(), palette::kBossHp, palette::kPanelDark, bossHpDelay_,
              ColorRGB(255, 190, 120));
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
