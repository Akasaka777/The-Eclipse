#include "UI/UpgradePanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <algorithm>
#include <cmath>

namespace ecl {
namespace ui {

namespace {
constexpr int   kVisibleRows = 9;
constexpr float kRowHeight = 66.0f;
} // namespace

UpgradePanel::UpgradePanel()
{
    Layout();
}

void UpgradePanel::Layout()
{
    window_ = Rect::FromXYWH(200.0f, 100.0f, 1520.0f, 880.0f);

    upgradeButton_ = Button(Rect::FromXYWH(window_.right - 420.0f, window_.bottom - 92.0f,
                                           240.0f, 62.0f), "強化する");
    upgradeButton_.SetAccent(palette::kAccentWarm);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 160.0f, window_.bottom - 92.0f,
                                         120.0f, 62.0f), "閉じる");
    filterButton_ = Button(Rect::FromXYWH(window_.left + 40.0f, window_.top + 74.0f, 220.0f, 44.0f),
                           "表示 : すべて", FontSize::Small);
}

void UpgradePanel::Open()
{
    open_ = true;
    closeRequested_ = false;
    selectedUid_ = 0;
    scroll_ = 0;
    resultTimer_ = 0.0f;
    message_.clear();
}

std::vector<const EquipmentItem*> UpgradePanel::SortedItems(const GameContext& context) const
{
    const Inventory& inventory = context.player.GetInventory();

    std::vector<const EquipmentItem*> items;
    for (const EquipmentItem& item : inventory.Items()) {
        if (equippedOnly_ && !inventory.IsEquipped(item.uid)) continue;
        items.push_back(&item);
    }

    std::sort(items.begin(), items.end(), [&inventory](const EquipmentItem* a, const EquipmentItem* b) {
        const bool ea = inventory.IsEquipped(a->uid);
        const bool eb = inventory.IsEquipped(b->uid);
        if (ea != eb) return ea;
        if (a->rarity != b->rarity) return static_cast<int>(a->rarity) > static_cast<int>(b->rarity);
        return a->Power() > b->Power();
    });
    return items;
}

void UpgradePanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;
    resultTimer_ = math::MaxF(0.0f, resultTimer_ - dt);

    Inventory& inventory = context.player.GetInventory();
    const std::vector<const EquipmentItem*> items = SortedItems(context);
    const int maxScroll = math::MaxI(0, static_cast<int>(items.size()) - kVisibleRows);

    const Rect listArea(window_.left + 40.0f, window_.top + 130.0f, window_.left + 740.0f,
                        window_.top + 130.0f + kRowHeight * static_cast<float>(kVisibleRows));

    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());

    if (listArea.Contains(mouseX, mouseY)) {
        const int wheel = input.WheelDelta();
        if (wheel != 0) scroll_ = math::ClampInt(scroll_ - wheel, 0, maxScroll);

        if (input.MouseClicked(MouseButton::Left)) {
            const int index = scroll_ + static_cast<int>((mouseY - listArea.top) / kRowHeight);
            if (index >= 0 && index < static_cast<int>(items.size())) {
                selectedUid_ = items[static_cast<size_t>(index)]->uid;
                resultTimer_ = 0.0f;
            }
        }
    }
    scroll_ = math::ClampInt(scroll_, 0, maxScroll);

    if (filterButton_.Update(input, dt)) {
        equippedOnly_ = !equippedOnly_;
        filterButton_.SetLabel(equippedOnly_ ? "表示 : 装備中のみ" : "表示 : すべて");
        scroll_ = 0;
    }

    // --- 強化実行 -----------------------------------------------------------
    const UpgradeCost cost = inventory.CalcUpgradeCost(selectedUid_);
    const bool affordable = cost.possible && inventory.Col() >= cost.col
                          && inventory.Material() >= cost.material;
    upgradeButton_.SetEnabled(affordable);

    if (upgradeButton_.Update(input, dt) && affordable) {
        bool success = false;
        if (inventory.TryUpgrade(selectedUid_, success)) {
            lastSuccess_ = success;
            resultTimer_ = 1.6f;
            message_ = success ? "強化成功！" : "強化失敗… 素材を消費しました";
        }
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void UpgradePanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "装備アップグレード");

    const Inventory& inventory = context.player.GetInventory();

    // --- 所持リソース --------------------------------------------------------
    draw::Text(FontSize::Normal, window_.right - 40.0f, window_.top + 74.0f, palette::kAccentWarm,
               str::Format("%s col", str::Comma(inventory.Col()).c_str()), draw::TextAlign::Right);
    draw::Text(FontSize::Normal, window_.right - 300.0f, window_.top + 74.0f, palette::kAccent,
               str::Format("強化結晶 %d", inventory.Material()), draw::TextAlign::Right);

    filterButton_.Draw();

    // --- 一覧 ---------------------------------------------------------------
    const std::vector<const EquipmentItem*> items = SortedItems(context);
    const Rect listArea(window_.left + 40.0f, window_.top + 130.0f, window_.left + 740.0f,
                        window_.top + 130.0f + kRowHeight * static_cast<float>(kVisibleRows));

    draw::FillRect(listArea.Expanded(6.0f), palette::kPanelDark, 190);
    draw::StrokeRect(listArea.Expanded(6.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    for (int row = 0; row < kVisibleRows; ++row) {
        const int index = scroll_ + row;
        if (index >= static_cast<int>(items.size())) break;

        const EquipmentItem& item = *items[static_cast<size_t>(index)];
        const Rect rect(listArea.left, listArea.top + kRowHeight * static_cast<float>(row),
                        listArea.right, listArea.top + kRowHeight * static_cast<float>(row + 1) - 6.0f);
        DrawItemRow(rect, item, item.uid == selectedUid_, inventory.IsEquipped(item.uid), false);
    }

    if (static_cast<int>(items.size()) > kVisibleRows) {
        const Rect track(listArea.right + 10.0f, listArea.top, listArea.right + 18.0f, listArea.bottom);
        draw::FillRect(track, palette::kPanelDark, 200);
        const float ratio = static_cast<float>(kVisibleRows) / static_cast<float>(items.size());
        const float offset = static_cast<float>(scroll_) / static_cast<float>(items.size());
        draw::FillRect(Rect(track.left, track.top + track.Height() * offset, track.right,
                            track.top + track.Height() * (offset + ratio)),
                       palette::kAccent, 220);
    }

    // --- 詳細 ---------------------------------------------------------------
    const Rect detail(window_.left + 790.0f, window_.top + 130.0f, window_.right - 40.0f,
                      window_.bottom - 110.0f);
    draw::FillRect(detail, palette::kPanelDark, 205);
    draw::StrokeRect(detail, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    const EquipmentItem* item = inventory.FindByUid(selectedUid_);
    if (!item) {
        draw::Text(FontSize::Normal, detail.CenterX(), detail.CenterY(), palette::kTextDisabled,
                   "強化する装備を選択してください", draw::TextAlign::Center);
        upgradeButton_.Draw();
        closeButton_.Draw();
        return;
    }

    float y = detail.top + 24.0f;
    draw::Text(FontSize::Large, detail.left + 24.0f, y, RarityColor(item->rarity), item->DisplayName());
    y += 58.0f;
    draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim,
               str::Format("%s ／ %s ／ 強化 %d / %d", RarityName(item->rarity),
                           item->IsWeapon() ? WeaponTypeName(item->weaponType) : EquipSlotName(item->slot),
                           item->upgradeLevel, item->MaxUpgrade()));
    y += 44.0f;

    // 強化段階のゲージ
    {
        const Rect bar(detail.left + 24.0f, y, detail.right - 24.0f, y + 16.0f);
        draw::FillRect(bar, palette::kPanel, 255);
        const int maxLevel = math::MaxI(1, item->MaxUpgrade());
        for (int i = 0; i < maxLevel; ++i) {
            const float w = bar.Width() / static_cast<float>(maxLevel);
            const Rect cell(bar.left + w * static_cast<float>(i) + 2.0f, bar.top + 2.0f,
                            bar.left + w * static_cast<float>(i + 1) - 2.0f, bar.bottom - 2.0f);
            draw::FillRect(cell, (i < item->upgradeLevel) ? palette::kAccentWarm : palette::kPanelDark, 255);
        }
        y += 40.0f;
    }

    // --- 強化後の変化 --------------------------------------------------------
    const Stats current = item->TotalStats();
    EquipmentItem preview = *item;
    preview.upgradeLevel = math::MinI(item->upgradeLevel + 1, item->MaxUpgrade());
    const Stats next = preview.TotalStats();

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "強化後の変化");
    y += 42.0f;
    if (item->IsWeapon()) {
        DrawStatDiffLine(detail.left + 24.0f, y, "攻撃力", current.attack, next.attack, false); y += 32.0f;
        DrawStatDiffLine(detail.left + 24.0f, y, "クリティカル率", current.critRate, next.critRate, true); y += 32.0f;
        DrawStatDiffLine(detail.left + 24.0f, y, "クリティカル倍率", current.critDamage, next.critDamage, true); y += 32.0f;
    } else {
        DrawStatDiffLine(detail.left + 24.0f, y, "防御力", current.defense, next.defense, false); y += 32.0f;
        DrawStatDiffLine(detail.left + 24.0f, y, "最大HP", current.maxHp, next.maxHp, false); y += 32.0f;
        DrawStatDiffLine(detail.left + 24.0f, y, "最大MP", current.maxMp, next.maxMp, false); y += 32.0f;
    }
    y += 22.0f;

    // --- コスト -------------------------------------------------------------
    const UpgradeCost cost = inventory.CalcUpgradeCost(selectedUid_);
    draw::Line(detail.left + 24.0f, y, detail.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;

    if (!cost.possible) {
        draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccentWarm, "強化上限に達しています");
    } else {
        const bool colOk = inventory.Col() >= cost.col;
        const bool matOk = inventory.Material() >= cost.material;

        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "必要 col");
        draw::Text(FontSize::Small, detail.left + 320.0f, y, colOk ? palette::kText : palette::kDanger,
                   str::Comma(cost.col), draw::TextAlign::Right);
        y += 32.0f;

        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "必要 強化結晶");
        draw::Text(FontSize::Small, detail.left + 320.0f, y, matOk ? palette::kText : palette::kDanger,
                   str::Format("%d", cost.material), draw::TextAlign::Right);
        y += 32.0f;

        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "成功率");
        const ColorRGB rateColor = (cost.successRate >= 0.99f) ? palette::kHp
                                 : (cost.successRate >= 0.6f ? palette::kAccentWarm : palette::kDanger);
        draw::Text(FontSize::Small, detail.left + 320.0f, y, rateColor,
                   str::Format("%.0f%%", cost.successRate * 100.0f), draw::TextAlign::Right);
        y += 40.0f;

        draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kTextDim,
                   "※ 失敗しても強化値は下がりません（col と結晶のみ消費）");
    }

    // --- 結果演出 -----------------------------------------------------------
    if (resultTimer_ > 0.0f) {
        const float t = math::Clamp(resultTimer_ / 1.6f, 0.0f, 1.0f);
        const ColorRGB color = lastSuccess_ ? palette::kAccentWarm : palette::kDanger;
        const float cx = detail.CenterX();
        const float cy = detail.top + 250.0f;

        if (lastSuccess_) {
            draw::Glow(cx, cy, 120.0f * (1.2f - t), color, static_cast<int>(200.0f * t), 5);
        }
        draw::TextAlpha(FontSize::Large, cx, cy - 24.0f, color, message_,
                        static_cast<int>(255.0f * t), draw::TextAlign::Center);
    }

    upgradeButton_.Draw();
    closeButton_.Draw();
}

} // namespace ui
} // namespace ecl
