#include "UI/SkillDraftPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Game/PlayerData.h"
#include "Game/Roguelike.h"
#include "Game/SwordSkill.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
constexpr float kInputDelay = 0.3f;   // 開いた直後の入力は受け付けない（攻撃の連打で決まらないように）

// UTF-8 の文字列を、幅に収まるように行へ分ける
std::vector<std::string> Wrap(FontSize size, const std::string& text, float maxWidth)
{
    std::vector<std::string> lines;
    std::string line;
    size_t i = 0;
    while (i < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        const size_t len = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        const std::string ch = text.substr(i, len);
        i += len;
        if (ch == "\n") {
            lines.push_back(line);
            line.clear();
            continue;
        }
        if (!line.empty() && static_cast<float>(draw::TextWidth(size, line + ch)) > maxWidth) {
            lines.push_back(line);
            line.clear();
        }
        line += ch;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

} // namespace

SkillDraftPanel::SkillDraftPanel()
{
    window_ = Rect::FromCenter(kScreenW * 0.5f, kScreenH * 0.5f, 1440.0f, 700.0f);
    const float cardW = 420.0f;
    const float gap = 36.0f;
    const float left = window_.CenterX() - (cardW * 3.0f + gap * 2.0f) * 0.5f;
    for (int i = 0; i < 3; ++i) {
        cards_.push_back(Rect::FromXYWH(left + (cardW + gap) * static_cast<float>(i), window_.top + 130.0f, cardW,
                                        420.0f));
    }
    const float slotW = 310.0f;
    const float slotGap = 24.0f;
    const float slotLeft = window_.CenterX() - (slotW * 4.0f + slotGap * 3.0f) * 0.5f;
    for (int i = 0; i < kSkillSlotCount; ++i) {
        slots_.push_back(Rect::FromXYWH(slotLeft + (slotW + slotGap) * static_cast<float>(i), window_.top + 250.0f,
                                        slotW, 200.0f));
    }
    skipButton_ = Button(Rect::FromXYWH(window_.CenterX() - 130.0f, window_.bottom - 92.0f, 260.0f, 60.0f),
                         "見送る", FontSize::Medium);
    backButton_ = Button(Rect::FromXYWH(window_.CenterX() - 130.0f, window_.bottom - 92.0f, 260.0f, 60.0f),
                         "選び直す", FontSize::Medium);
}

void SkillDraftPanel::Open(const std::vector<int>& choices)
{
    choices_ = choices;
    if (choices_.size() > cards_.size()) choices_.resize(cards_.size());
    step_ = Step::Choose;
    selected_ = 0;
    slotSelected_ = 0;
    open_ = !choices_.empty();
    changed_ = false;
    openTimer_ = 0.0f;
    message_.clear();
}

void SkillDraftPanel::Pick(PlayerData& data, int index)
{
    if (index < 0 || index >= static_cast<int>(choices_.size())) return;
    const int id = choices_[static_cast<size_t>(index)];
    const SwordSkill* skill = SkillDatabase::Instance().Find(id);
    const std::string name = skill ? skill->name : "スキル";
    switch (rogue::ApplySkillPick(data, id)) {
    case rogue::SkillPick::Learned:
        message_ = str::Format("%s を覚えた", name.c_str());
        changed_ = true;
        open_ = false;
        break;
    case rogue::SkillPick::LevelUp:
        message_ = str::Format("%s が Lv %d になった", name.c_str(), data.SkillLevel(id));
        changed_ = true;
        open_ = false;
        break;
    case rogue::SkillPick::NeedSlot:
        // スロットが埋まっているので、入れ替えるスキルを選んでもらう
        selected_ = index;
        step_ = Step::Replace;
        slotSelected_ = 0;
        break;
    default:
        break;
    }
}

void SkillDraftPanel::Replace(PlayerData& data, int slot)
{
    const int id = choices_[static_cast<size_t>(selected_)];
    const SwordSkill* old = data.SkillAt(slot);
    const std::string oldName = old ? old->name : "";
    if (rogue::ApplySkillPick(data, id, slot) != rogue::SkillPick::Learned) return;
    const SwordSkill* skill = SkillDatabase::Instance().Find(id);
    message_ = str::Format("%s を捨てて %s を覚えた", oldName.c_str(), skill ? skill->name.c_str() : "");
    changed_ = true;
    open_ = false;
}

void SkillDraftPanel::Update(float dt, const Input& input, PlayerData& data)
{
    if (!open_) return;
    openTimer_ += dt;
    const bool ready = openTimer_ >= kInputDelay;
    const float mx = static_cast<float>(input.MouseX());
    const float my = static_cast<float>(input.MouseY());
    const bool clicked = ready && input.MouseClicked(MouseButton::Left);

    if (step_ == Step::Choose) {
        const int count = static_cast<int>(choices_.size());
        if (input.Pressed(GameAction::MoveLeft)) selected_ = (selected_ + count - 1) % count;
        if (input.Pressed(GameAction::MoveRight)) selected_ = (selected_ + 1) % count;
        for (int i = 0; i < count; ++i) {
            if (cards_[static_cast<size_t>(i)].Contains(mx, my)) {
                selected_ = i;
                if (clicked) {
                    Pick(data, i);
                    return;
                }
            }
        }
        if (skipButton_.Update(input, dt) && ready) {
            message_ = "スキルを見送った";
            open_ = false;
            return;
        }
        if (ready && input.Pressed(GameAction::Confirm)) Pick(data, selected_);
        else if (ready && input.Pressed(GameAction::Cancel)) {
            message_ = "スキルを見送った";
            open_ = false;
        }
        return;
    }

    // 入れ替えるスロットを選ぶ
    const int limit = data.SkillSlotLimit();
    if (input.Pressed(GameAction::MoveLeft)) slotSelected_ = (slotSelected_ + limit - 1) % limit;
    if (input.Pressed(GameAction::MoveRight)) slotSelected_ = (slotSelected_ + 1) % limit;
    for (int i = 0; i < limit; ++i) {
        if (slots_[static_cast<size_t>(i)].Contains(mx, my)) {
            slotSelected_ = i;
            if (clicked) {
                Replace(data, i);
                return;
            }
        }
    }
    if (backButton_.Update(input, dt) || (ready && input.Pressed(GameAction::Cancel))) {
        step_ = Step::Choose;
        return;
    }
    if (ready && input.Pressed(GameAction::Confirm)) Replace(data, slotSelected_);
}

void SkillDraftPanel::Draw(const PlayerData& data) const
{
    if (!open_) return;
    DrawDimOverlay(170);
    draw::Panel(window_, palette::kPanel, palette::kAccent, 245, 2.0f);
    const ColorRGB glow(150, 240, 255);

    if (step_ == Step::Choose) {
        draw::Text(FontSize::Large, window_.CenterX(), window_.top + 28.0f, glow, "スキルを 1 つ選ぶ",
                   draw::TextAlign::Center);
        draw::Text(FontSize::Small, window_.CenterX(), window_.top + 84.0f, palette::kTextDim,
                   str::Format("新しいスキルはスロットにセット ／ 持っているスキルを選ぶと Lv が上がる（最大 Lv %d）",
                               kMaxSkillLevel),
                   draw::TextAlign::Center);

        for (size_t i = 0; i < choices_.size(); ++i) {
            const Rect& card = cards_[i];
            const SwordSkill* skill = SkillDatabase::Instance().Find(choices_[i]);
            if (!skill) continue;
            const bool selected = (static_cast<int>(i) == selected_);
            const bool owned = data.IsSkillEquipped(skill->id);
            const int level = owned ? data.SkillLevel(skill->id) : 0;

            draw::GradientRectV(card, skill->effectColor.Scaled(selected ? 0.42f : 0.22f), palette::kPanelDark, 240, 12);
            draw::StrokeRect(card, selected ? skill->effectColor : palette::kBorder.Scaled(0.8f), selected ? 3.0f : 1.0f, 255);
            if (selected) draw::StrokeRect(card.Expanded(5.0f), skill->effectColor, 1.0f, 140);

            // 新しい / Lv アップの札
            const Rect badge = Rect::FromXYWH(card.left + 18.0f, card.top + 18.0f, owned ? 200.0f : 90.0f, 34.0f);
            draw::FillRect(badge, owned ? palette::kExp.Scaled(0.35f) : palette::kHp.Scaled(0.35f), 240);
            draw::StrokeRect(badge, owned ? palette::kExp : palette::kHp, 1.0f, 240);
            draw::Text(FontSize::Small, badge.CenterX(), badge.top + 5.0f, owned ? palette::kExp : palette::kHp,
                       owned ? str::Format("Lv %d → Lv %d", level, level + 1) : std::string("NEW"),
                       draw::TextAlign::Center);

            draw::Text(FontSize::Medium, card.left + 20.0f, card.top + 70.0f, palette::kText, skill->name);
            float y = card.top + 124.0f;
            for (const std::string& line : Wrap(FontSize::Small, skill->description, card.Width() - 40.0f)) {
                draw::Text(FontSize::Small, card.left + 20.0f, y, palette::kTextDim, line);
                y += 28.0f;
                if (y > card.top + 250.0f) break;
            }

            // 威力と再使用までの時間（Lv が上がるときは変化も出す）
            const int now = owned ? level : 1;
            const int next = owned ? level + 1 : 1;
            float row = card.bottom - 132.0f;
            draw::Line(card.left + 20.0f, row - 10.0f, card.right - 20.0f, row - 10.0f, palette::kBorder, 1.0f, 140);
            auto stat = [&](const char* label, const std::string& value) {
                draw::Text(FontSize::Small, card.left + 20.0f, row, palette::kTextDim, label);
                draw::Text(FontSize::Small, card.right - 20.0f, row, palette::kText, value, draw::TextAlign::Right);
                row += 32.0f;
            };
            const float power = skill->TotalMultiplier();
            stat("威力（合計倍率）", owned ? str::Format("×%.2f → ×%.2f", power * SkillLevelDamageRate(now),
                                                       power * SkillLevelDamageRate(next))
                                          : str::Format("×%.2f", power));
            stat("再使用まで", owned ? str::Format("%.1f 秒 → %.1f 秒", skill->cooldown * SkillLevelCooldownRate(now),
                                                  skill->cooldown * SkillLevelCooldownRate(next))
                                     : str::Format("%.1f 秒", skill->cooldown));
            stat("消費 MP", str::Format("%d", static_cast<int>(skill->mpCost)));
        }

        // いまのスロット
        std::string owned = "スロット：";
        for (int i = 0; i < data.SkillSlotLimit(); ++i) {
            const SwordSkill* s = data.SkillAt(i);
            owned += (i > 0 ? " ／ " : "") + (s ? str::Format("%s Lv %d", s->name.c_str(), data.SkillLevel(s->id))
                                                : std::string("（空き）"));
        }
        draw::Text(FontSize::Tiny, window_.CenterX(), window_.bottom - 132.0f, palette::kTextDim, owned,
                   draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, window_.right - 30.0f, window_.bottom - 30.0f, palette::kTextDisabled,
                   "←→ で選ぶ ／ ENTER・クリックで決定 ／ ESC で見送る", draw::TextAlign::Right);
        skipButton_.Draw();
        return;
    }

    // --- 入れ替えるスキルを選ぶ ---------------------------------------------------
    const SwordSkill* picked = SkillDatabase::Instance().Find(choices_[static_cast<size_t>(selected_)]);
    draw::Text(FontSize::Large, window_.CenterX(), window_.top + 28.0f, glow, "入れ替えるスキルを選ぶ",
               draw::TextAlign::Center);
    draw::Text(FontSize::Small, window_.CenterX(), window_.top + 96.0f, palette::kTextDim,
               str::Format("スロットが埋まっています。「%s」と入れ替えるスキルを選んでください（選んだスキルは捨てます）",
                           picked ? picked->name.c_str() : ""),
               draw::TextAlign::Center);
    for (int i = 0; i < data.SkillSlotLimit(); ++i) {
        const Rect& slot = slots_[static_cast<size_t>(i)];
        const SwordSkill* s = data.SkillAt(i);
        const bool selected = (i == slotSelected_);
        const ColorRGB color = s ? s->effectColor : palette::kTextDisabled;
        draw::GradientRectV(slot, color.Scaled(selected ? 0.40f : 0.18f), palette::kPanelDark, 240, 10);
        draw::StrokeRect(slot, selected ? palette::kDanger : palette::kBorder.Scaled(0.8f), selected ? 3.0f : 1.0f, 255);
        draw::Text(FontSize::Small, slot.left + 16.0f, slot.top + 14.0f, palette::kTextDim,
                   str::Format("スロット %d", i + 1));
        draw::Text(FontSize::Medium, slot.left + 16.0f, slot.top + 60.0f, palette::kText, s ? s->name : "（空き）");
        if (s) {
            draw::Text(FontSize::Small, slot.left + 16.0f, slot.top + 112.0f, palette::kExp,
                       str::Format("Lv %d", data.SkillLevel(s->id)));
        }
        if (selected) {
            draw::Text(FontSize::Small, slot.CenterX(), slot.bottom + 12.0f, palette::kDanger, "これを捨てる",
                       draw::TextAlign::Center);
        }
    }
    draw::Text(FontSize::Tiny, window_.right - 30.0f, window_.bottom - 30.0f, palette::kTextDisabled,
               "←→ で選ぶ ／ ENTER・クリックで入れ替え ／ ESC で選び直す", draw::TextAlign::Right);
    backButton_.Draw();
}

} // namespace ui
} // namespace ecl
