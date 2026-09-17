#include "UI/BattleMenu.h"

#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

BattleMenu::BattleMenu()
{
    window_ = Rect::FromXYWH(660.0f, 260.0f, 600.0f, 560.0f);

    const float x = window_.left + 100.0f;
    resumeButton_ = Button(Rect::FromXYWH(x, window_.top + 100.0f, 400.0f, 72.0f), "ゲームに戻る");
    equipButton_ = Button(Rect::FromXYWH(x, window_.top + 190.0f, 400.0f, 72.0f), "装備");
    settingsButton_ = Button(Rect::FromXYWH(x, window_.top + 280.0f, 400.0f, 72.0f), "設定");
    retireButton_ = Button(Rect::FromXYWH(x, window_.top + 370.0f, 400.0f, 72.0f), "クエストリタイア");
    retireButton_.SetAccent(palette::kDanger);

    retireYesButton_ = Button(Rect::FromXYWH(window_.CenterX() - 210.0f, window_.bottom - 150.0f,
                                            190.0f, 64.0f), "リタイアする");
    retireYesButton_.SetAccent(palette::kDanger);
    retireNoButton_ = Button(Rect::FromXYWH(window_.CenterX() + 20.0f, window_.bottom - 150.0f,
                                            190.0f, 64.0f), "戻る");
}

void BattleMenu::Open()
{
    open_ = true;
    confirmingRetire_ = false;
    retireConfirmed_ = false;
    equipmentChanged_ = false;
}

void BattleMenu::Close()
{
    open_ = false;
    confirmingRetire_ = false;
    settings_.Close();
    equipment_.Close();
}

void BattleMenu::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    retireConfirmed_ = false;
    equipmentChanged_ = false;

    // 装備パネルが開いている間はそちらを優先
    if (equipment_.IsOpen()) {
        equipment_.Update(dt, input, context);
        equipmentChanged_ = equipment_.EquipmentChanged();
        return;
    }

    // 設定パネルが開いている間はそちらを優先
    if (settings_.IsOpen()) {
        settings_.Update(dt, input, context);
        return;
    }

    if (confirmingRetire_) {
        if (retireYesButton_.Update(input, dt)) {
            retireConfirmed_ = true;
            open_ = false;
        }
        if (retireNoButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
            confirmingRetire_ = false;
        }
        return;
    }

    if (resumeButton_.Update(input, dt) || input.Pressed(GameAction::Menu)) {
        Close();
        return;
    }
    if (equipButton_.Update(input, dt)) {
        equipment_.Open();
        // 戦闘中は売却させない（誤操作防止）
        equipment_.SetSellEnabled(false);
    }
    if (settingsButton_.Update(input, dt)) {
        settings_.Open(context.settings);
    }
    if (retireButton_.Update(input, dt)) {
        confirmingRetire_ = true;
    }
}

void BattleMenu::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawDimOverlay(150);

    if (equipment_.IsOpen()) {
        equipment_.Draw(context);
        return;
    }
    if (settings_.IsOpen()) {
        settings_.Draw();
        return;
    }

    DrawWindow(window_, "MENU");

    if (confirmingRetire_) {
        draw::Text(FontSize::Medium, window_.CenterX(), window_.top + 160.0f, palette::kText,
                   "クエストを中断しますか？", draw::TextAlign::Center);
        draw::Text(FontSize::Small, window_.CenterX(), window_.top + 220.0f, palette::kTextDim,
                   "入手したドロップの一部は失われます", draw::TextAlign::Center);
        retireYesButton_.Draw();
        retireNoButton_.Draw();
        return;
    }

    resumeButton_.Draw();
    equipButton_.Draw();
    settingsButton_.Draw();
    retireButton_.Draw();

    // 装備が壊れたときに交換できることを伝える
    draw::Text(FontSize::Tiny, window_.CenterX(), window_.bottom - 44.0f, palette::kTextDim,
               "装備が壊れたら「装備」から替えに変更できます", draw::TextAlign::Center);
}

} // namespace ui
} // namespace ecl
