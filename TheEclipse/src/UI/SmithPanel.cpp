#include "UI/SmithPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <algorithm>
#include <cmath>

namespace ecl {
namespace ui {

namespace {

// 装備の種類で絞り込むタブ（先頭は「すべて」）
struct CategoryDef
{
    const char* label;
    EquipSlot   slot;   // Count なら「すべて」
};

const CategoryDef kCategories[] = {
    { "すべて", EquipSlot::Count },
    { "武器",   EquipSlot::WeaponRight },
    { "頭装備", EquipSlot::Head },
    { "体装備", EquipSlot::Body },
    { "盾",     EquipSlot::Shield },
    { "腕装備", EquipSlot::Arm },
    { "手装備", EquipSlot::Hand },
    { "足装備", EquipSlot::Foot },
};

constexpr int kCategoryCount = static_cast<int>(sizeof(kCategories) / sizeof(kCategories[0]));

} // namespace

namespace {
constexpr int   kVisibleRows = 9;
constexpr float kRowHeight = 66.0f;
} // namespace

SmithPanel::SmithPanel()
{
    Layout();
}

void SmithPanel::Layout()
{
    window_ = Rect::FromXYWH(200.0f, 100.0f, 1520.0f, 880.0f);

    const float buttonY = window_.bottom - 92.0f;
    upgradeButton_ = Button(Rect::FromXYWH(window_.right - 820.0f, buttonY, 200.0f, 62.0f),
                            "強化する");
    upgradeButton_.SetAccent(palette::kAccentWarm);
    repairButton_ = Button(Rect::FromXYWH(window_.right - 600.0f, buttonY, 200.0f, 62.0f),
                           "修理する");
    repairButton_.SetAccent(palette::kHp);
    repairAllButton_ = Button(Rect::FromXYWH(window_.right - 380.0f, buttonY, 200.0f, 62.0f),
                              "すべて修理");
    repairAllButton_.SetAccent(palette::kHp);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 160.0f, buttonY, 120.0f, 62.0f), "閉じる");
    filterButton_ = Button(Rect::FromXYWH(window_.left + 40.0f, window_.top + 74.0f, 220.0f, 44.0f),
                           "表示 : すべて", FontSize::Small);

    // --- 装備の種類で絞り込むタブ（「表示 : ...」の右隣に並べる）-----------------
    categoryButtons_.clear();
    const float tabWidth = 90.0f;
    const float tabGap = 6.0f;
    const float tabLeft = filterButton_.GetRect().right + 12.0f;
    for (int i = 0; i < kCategoryCount; ++i) {
        const Rect rect = Rect::FromXYWH(tabLeft + (tabWidth + tabGap) * static_cast<float>(i),
                                         window_.top + 74.0f, tabWidth, 44.0f);
        categoryButtons_.push_back(Button(rect, kCategories[i].label, FontSize::Tiny));
    }

    // --- 使う強化結晶の個数 -----------------------------------------------------
    const float detailLeft = window_.left + 790.0f;
    const float crystalY = window_.top + 130.0f + 490.0f;
    crystalMinusButton_ = Button(Rect::FromXYWH(detailLeft + 250.0f, crystalY, 44.0f, 34.0f),
                                 "-", FontSize::Small);
    crystalPlusButton_ = Button(Rect::FromXYWH(detailLeft + 348.0f, crystalY, 44.0f, 34.0f),
                                "+", FontSize::Small);
}

void SmithPanel::Open()
{
    open_ = true;
    closeRequested_ = false;
    selectedUid_ = 0;
    scroll_ = 0;
    resultTimer_ = 0.0f;
    crystals_ = 1;
    lastResultUid_ = 0;
    message_.clear();
}

std::vector<const EquipmentItem*> SmithPanel::SortedItems(const GameContext& context) const
{
    const Inventory& inventory = context.player.GetInventory();

    std::vector<const EquipmentItem*> items;
    const EquipSlot category = kCategories[math::ClampInt(categoryIndex_, 0, kCategoryCount - 1)].slot;
    for (const EquipmentItem& item : inventory.Items()) {
        if (equippedOnly_ && !inventory.IsEquipped(item.uid)) continue;
        // 武器タブは左右どちらの手の武器もまとめて出す
        if (category != EquipSlot::Count && item.slot != category) continue;
        items.push_back(&item);
    }

    std::sort(items.begin(), items.end(), [&inventory](const EquipmentItem* a, const EquipmentItem* b) {
        const bool ea = inventory.IsEquipped(a->uid);
        const bool eb = inventory.IsEquipped(b->uid);
        if (ea != eb) return ea;
        if (a->iv != b->iv) return a->iv > b->iv;
        return a->Power() > b->Power();
    });
    return items;
}

void SmithPanel::Update(float dt, const Input& input, GameContext& context)
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
                if (selectedUid_ != items[static_cast<size_t>(index)]->uid) lastResultUid_ = 0;
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

    // --- 装備の種類タブ -------------------------------------------------------
    for (int i = 0; i < static_cast<int>(categoryButtons_.size()); ++i) {
        categoryButtons_[static_cast<size_t>(i)].SetSelected(i == categoryIndex_);
        if (categoryButtons_[static_cast<size_t>(i)].Update(input, dt)) {
            categoryIndex_ = i;
            scroll_ = 0;
        }
    }

    // --- 使う強化結晶の個数 ---------------------------------------------------
    crystals_ = math::ClampInt(crystals_, kMinUpgradeCrystals, kMaxUpgradeCrystals);
    crystalMinusButton_.SetEnabled(crystals_ > kMinUpgradeCrystals);
    crystalPlusButton_.SetEnabled(crystals_ < kMaxUpgradeCrystals);
    if (crystalMinusButton_.Update(input, dt) && crystalMinusButton_.Enabled()) --crystals_;
    if (crystalPlusButton_.Update(input, dt) && crystalPlusButton_.Enabled()) ++crystals_;

    // --- 強化実行 -----------------------------------------------------------
    const UpgradeCost cost = inventory.CalcUpgradeCost(selectedUid_, crystals_);
    const bool affordable = cost.possible && inventory.Col() >= cost.col
                          && inventory.Material() >= cost.material;
    upgradeButton_.SetEnabled(affordable);

    if (upgradeButton_.Update(input, dt) && affordable) {
        UpgradeResult result;
        if (inventory.TryUpgrade(selectedUid_, crystals_, result)) {
            lastSuccess_ = result.success;
            resultTimer_ = 1.6f;
            // 伸び率は強化してから分かる
            lastResultUid_ = result.success ? selectedUid_ : 0;
            lastBefore_ = result.before;
            lastAfter_ = result.after;
            message_ = result.success ? "強化成功！" : "強化失敗… 素材を消費しました";
        }
    }

    // --- 修理 ---------------------------------------------------------------
    const int repairCost = inventory.RepairCost(selectedUid_);
    repairButton_.SetEnabled(repairCost > 0 && inventory.Col() >= repairCost);
    if (repairButton_.Update(input, dt) && repairButton_.Enabled()) {
        if (inventory.Repair(selectedUid_)) {
            lastSuccess_ = true;
            resultTimer_ = 1.6f;
            message_ = str::Format("修理しました（-%s col）", str::Comma(repairCost).c_str());
        }
    }

    const int repairAllCost = inventory.RepairAllCost();
    repairAllButton_.SetEnabled(repairAllCost > 0 && inventory.Col() > 0);
    if (repairAllButton_.Update(input, dt) && repairAllButton_.Enabled()) {
        const int before = inventory.Col();
        const int repaired = inventory.RepairAll();
        if (repaired > 0) {
            lastSuccess_ = true;
            resultTimer_ = 1.6f;
            message_ = str::Format("%d 点を修理しました（-%s col）", repaired,
                                   str::Comma(before - inventory.Col()).c_str());
        } else {
            message_ = "col が足りません";
            resultTimer_ = 1.6f;
            lastSuccess_ = false;
        }
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void SmithPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "鍛冶屋");

    const Inventory& inventory = context.player.GetInventory();

    // --- 所持リソース --------------------------------------------------------
    draw::Text(FontSize::Normal, window_.right - 40.0f, window_.top + 74.0f, palette::kAccentWarm,
               str::Format("%s col", str::Comma(inventory.Col()).c_str()), draw::TextAlign::Right);
    draw::Text(FontSize::Normal, window_.right - 300.0f, window_.top + 74.0f, palette::kAccent,
               str::Format("強化結晶 %d", inventory.Material()), draw::TextAlign::Right);

    filterButton_.Draw();
    for (const Button& button : categoryButtons_) button.Draw();

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
        repairButton_.Draw();
        repairAllButton_.Draw();
        closeButton_.Draw();
        DrawRepairSummary(context);
        return;
    }

    float y = detail.top + 24.0f;
    draw::Text(FontSize::Large, detail.left + 24.0f, y, palette::kAccent, item->DisplayName());
    y += 58.0f;
    draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim,
               str::Format("%s ／ 強化 %d / %d",
                           item->IsWeapon() ? WeaponTypeName(item->weaponType) : EquipSlotName(item->slot),
                           item->upgradeLevel, item->MaxUpgrade()));
    y += 30.0f;
    draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim,
               str::Format("戦力 %d ／ 耐久力 最大 %d", item->Power(),
                           item->MaxDurabilityDisplay()));
    y += 36.0f;

    // --- 耐久力 ---------------------------------------------------------------
    {
        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "耐久力");
        const Rect bar(detail.left + 140.0f, y + 4.0f, detail.right - 160.0f, y + 20.0f);
        draw::FillRect(bar, palette::kPanelDark, 255);
        draw::Bar(bar, item->DurabilityRatio(), item->DurabilityColor(), ColorRGB(38, 40, 50));
        draw::StrokeRect(bar, palette::kBorder.Scaled(0.7f), 1.0f, 160);
        draw::Text(FontSize::Small, detail.right - 24.0f, y, item->DurabilityColor(),
                   str::Format("%d / %d", item->DurabilityDisplay(), item->MaxDurabilityDisplay()),
                   draw::TextAlign::Right);
        y += 34.0f;

        const int cost = inventory.RepairCost(item->uid);
        if (cost > 0) {
            draw::Text(FontSize::Tiny, detail.left + 24.0f, y,
                       inventory.Col() >= cost ? palette::kTextDim : palette::kDanger,
                       str::Format("修理費用  %s col", str::Comma(cost).c_str()));
        } else {
            draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kHp, "修理の必要はありません");
        }
        y += 34.0f;
    }

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

    // --- 強化による伸び ------------------------------------------------------
    //   攻撃力・クリティカル率・クリティカル倍率はランダムなので、
    //   強化するまでは「???」。強化すると「強化前 → 強化後」を表示する。
    const Stats current = item->TotalStats();
    const bool hasResult = (lastResultUid_ == selectedUid_ && selectedUid_ != 0);
    const Stats shownBefore = hasResult ? lastBefore_ : current;
    const Stats shownAfter = hasResult ? lastAfter_ : current;

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "強化による伸び");
    draw::Text(FontSize::Tiny, detail.right - 24.0f, y + 6.0f, palette::kTextDim,
               hasResult ? "前回の強化結果" : "強化するまで分かりません", draw::TextAlign::Right);
    y += 40.0f;

    DrawGrowthLine(detail.left + 24.0f, y, "攻撃力", shownBefore.attack, shownAfter.attack,
                   hasResult, false);
    y += 30.0f;
    DrawGrowthLine(detail.left + 24.0f, y, "クリティカル率", shownBefore.critRate,
                   shownAfter.critRate, hasResult, true);
    y += 30.0f;
    DrawGrowthLine(detail.left + 24.0f, y, "クリティカル倍率", shownBefore.critDamage,
                   shownAfter.critDamage, hasResult, true);
    y += 34.0f;

    // 防御力などは強化値どおりに伸びるので、予測を出す
    {
        EquipmentItem preview = *item;
        preview.upgradeLevel = math::MinI(item->upgradeLevel + 1, item->MaxUpgrade());
        const Stats next = preview.TotalStats();
        DrawStatDiffLine(detail.left + 24.0f, y, "防御力", current.defense, next.defense, false);
        y += 28.0f;
        DrawStatDiffLine(detail.left + 24.0f, y, "最大HP", current.maxHp, next.maxHp, false);
        y += 28.0f;
        DrawStatDiffLine(detail.left + 24.0f, y, "最大MP", current.maxMp, next.maxMp, false);
        y += 30.0f;
    }

    // --- コスト -------------------------------------------------------------
    const UpgradeCost cost = inventory.CalcUpgradeCost(selectedUid_, crystals_);
    draw::Line(detail.left + 24.0f, y, detail.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 14.0f;

    if (!cost.possible) {
        draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccentWarm,
                   "強化上限に達しています");
    } else {
        const bool colOk = inventory.Col() >= cost.col;
        const bool matOk = inventory.Material() >= cost.material;

        // 使う強化結晶の個数（多いほど伸びも費用も大きくなる）
        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "使う強化結晶");
        crystalMinusButton_.Draw();
        crystalPlusButton_.Draw();
        draw::Text(FontSize::Normal, detail.left + 320.0f, y - 2.0f,
                   matOk ? palette::kText : palette::kDanger,
                   str::Format("%d", crystals_), draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, detail.left + 404.0f, y + 4.0f, palette::kTextDim,
                   str::Format("／ 最大 %d（所持 %d）", kMaxUpgradeCrystals, inventory.Material()));
        y += 44.0f;

        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "伸びる割合");
        draw::Text(FontSize::Small, detail.left + 360.0f, y, palette::kAccentWarm,
                   str::Format("%.1f%% 〜 %.1f%%", cost.minGrowth * 100.0f,
                               cost.maxGrowth * 100.0f), draw::TextAlign::Right);
        y += 30.0f;

        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "必要 col");
        draw::Text(FontSize::Small, detail.left + 360.0f, y, colOk ? palette::kText : palette::kDanger,
                   str::Comma(cost.col), draw::TextAlign::Right);
        y += 30.0f;

        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, "成功率");
        const ColorRGB rateColor = (cost.successRate >= 0.99f) ? palette::kHp
                                 : (cost.successRate >= 0.6f ? palette::kAccentWarm : palette::kDanger);
        draw::Text(FontSize::Small, detail.left + 360.0f, y, rateColor,
                   str::Format("%.0f%%", cost.successRate * 100.0f), draw::TextAlign::Right);
        y += 28.0f;

        draw::Text(FontSize::Tiny, detail.left + 24.0f, y, palette::kTextDim,
                   "※ 伸び率は 3 つとも別々に抽選されます（失敗しても強化値は下がりません）");
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
    repairButton_.Draw();
    repairAllButton_.Draw();
    closeButton_.Draw();
    DrawRepairSummary(context);
}

void SmithPanel::DrawGrowthLine(float x, float y, const char* label, float before, float after,
                               bool hasResult, bool percent) const
{
    auto format = [percent](float value) {
        return percent ? str::Format("%.1f%%", value * 100.0f)
                       : str::Format("%d", static_cast<int>(value));
    };

    draw::Text(FontSize::Small, x, y, palette::kTextDim, label);
    draw::Text(FontSize::Small, x + 300.0f, y, palette::kText, format(before),
               draw::TextAlign::Right);
    draw::Text(FontSize::Small, x + 324.0f, y, palette::kTextDim, "→");

    if (!hasResult) {
        draw::Text(FontSize::Small, x + 470.0f, y, palette::kTextDisabled, "???",
                   draw::TextAlign::Right);
        return;
    }

    const bool up = (after > before + 0.0001f);
    draw::Text(FontSize::Small, x + 470.0f, y, up ? palette::kHp : palette::kText, format(after),
               draw::TextAlign::Right);
}

void SmithPanel::DrawRepairSummary(const GameContext& context) const
{
    const Inventory& inventory = context.player.GetInventory();
    const int allCost = inventory.RepairAllCost();

    const Rect info(window_.left + 40.0f, window_.bottom - 92.0f, window_.right - 840.0f,
                    window_.bottom - 30.0f);
    draw::FillRect(info, palette::kPanelDark, 190);
    draw::StrokeRect(info, palette::kBorder.Scaled(0.6f), 1.0f, 150);

    if (allCost > 0) {
        draw::Text(FontSize::Small, info.left + 14.0f, info.top + 6.0f, palette::kTextDim,
                   "すべて修理");
        draw::Text(FontSize::Small, info.right - 14.0f, info.top + 6.0f,
                   inventory.Col() >= allCost ? palette::kText : palette::kDanger,
                   str::Format("%s col", str::Comma(allCost).c_str()), draw::TextAlign::Right);
        draw::Text(FontSize::Tiny, info.left + 14.0f, info.top + 34.0f, palette::kTextDim,
                   "耐久力が 0 になった装備は失われます");
    } else {
        draw::Text(FontSize::Small, info.left + 14.0f, info.CenterY() - 10.0f, palette::kHp,
                   "すべての装備が万全の状態です");
    }
}

} // namespace ui
} // namespace ecl
