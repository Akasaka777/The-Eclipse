#include "Scenes/HomeScene.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Game/GameContext.h"
#include "Game/QuestDatabase.h"
#include "Game/SaveData.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
constexpr float kTabBarHeight = 104.0f;
constexpr float kFieldWidth = 2600.0f;
constexpr float kGateX = 2300.0f;

struct TabDef
{
    HomeTab tab;
    const char* label;
};

const TabDef kTabs[] = {
    { HomeTab::Equipment, "装備" },
    { HomeTab::Skill,     "スキル" },
    { HomeTab::Quest,     "クエスト" },
    { HomeTab::Upgrade,   "強化" },
    { HomeTab::Settings,  "設定" },
};

constexpr int kTabCount = static_cast<int>(sizeof(kTabs) / sizeof(kTabs[0]));

} // namespace

HomeScene::HomeScene()
{
    const float buttonWidth = 270.0f;
    const float spacing = 20.0f;
    const float totalWidth = buttonWidth * static_cast<float>(kTabCount)
                           + spacing * static_cast<float>(kTabCount - 1);
    const float startX = (kScreenW - totalWidth) * 0.5f;

    for (int i = 0; i < kTabCount; ++i) {
        const Rect rect = Rect::FromXYWH(startX + (buttonWidth + spacing) * static_cast<float>(i),
                                         kScreenH - kTabBarHeight + 18.0f, buttonWidth, 68.0f);
        ui::Button button(rect, kTabs[i].label, FontSize::Medium);
        tabButtons_.push_back(button);
    }
}

void HomeScene::BuildField()
{
    FloorDef field;
    field.name = "HOME";
    field.width = kFieldWidth;
    field.groundY = 880.0f;
    field.depth = config::kDefaultFieldDepth;
    field.theme = StageTheme::Home;
    field.platforms = {
        Platform(620.0f, 700.0f, 300.0f, 28.0f),
        Platform(1180.0f, 620.0f, 280.0f, 28.0f),
        Platform(1720.0f, 700.0f, 300.0f, 28.0f),
    };
    stage_.Load(field);
    stage_.SetGateOpen(true);
}

void HomeScene::OnEnter(GameContext& context)
{
    BuildField();

    player_.Setup(context.player);
    const float startZ = stage_.Depth() * 0.5f;
    player_.PlaceAt(Vec2(360.0f, stage_.GroundYAt(startZ)));
    player_.z = startZ;
    player_.FullHeal();

    camera_.Reset();
    camera_.SetWorldBounds(0.0f, kFieldWidth, 0.0f, kScreenH);
    camera_.SetTarget(Vec2(player_.pos.x, kScreenH * 0.5f));
    camera_.SnapToTarget();
    camera_.SetShakeEnabled(context.settings.screenShake);

    combat_.Clear();
    activeTab_ = HomeTab::None;
    CloseAllTabs();
    startQuest_ = false;
    time_ = 0.0f;

    // ホームに戻ったタイミングで自動セーブ
    saveNoticeTimer_ = SaveSystem::Save(context) ? 2.6f : 0.0f;
}

bool HomeScene::AnyPanelOpen() const
{
    return equipPanel_.IsOpen() || skillPanel_.IsOpen() || questPanel_.IsOpen()
        || upgradePanel_.IsOpen() || settingsPanel_.IsOpen();
}

void HomeScene::CloseAllTabs()
{
    equipPanel_.Close();
    skillPanel_.Close();
    questPanel_.Close();
    upgradePanel_.Close();
    settingsPanel_.Close();
    activeTab_ = HomeTab::None;
}

void HomeScene::OpenTab(HomeTab tab, GameContext& context)
{
    CloseAllTabs();
    activeTab_ = tab;

    switch (tab) {
    case HomeTab::Equipment: equipPanel_.Open(); break;
    case HomeTab::Skill:     skillPanel_.Open(context); break;
    case HomeTab::Quest:     questPanel_.Open(context); break;
    case HomeTab::Upgrade:   upgradePanel_.Open(); break;
    case HomeTab::Settings:  settingsPanel_.Open(context.settings); break;
    default: break;
    }
}

void HomeScene::Update(float dt, GameContext& context, SceneManager& manager)
{
    time_ += dt;
    saveNoticeTimer_ = math::MaxF(0.0f, saveNoticeTimer_ - dt);
    const Input& input = Input::Instance();

    // --- パネル操作 ----------------------------------------------------------
    if (equipPanel_.IsOpen()) {
        equipPanel_.Update(dt, input, context);
        // 装備変更を見た目へ即反映
        player_.Setup(context.player);
        player_.FullHeal();
        if (equipPanel_.CloseRequested()) activeTab_ = HomeTab::None;
    } else if (skillPanel_.IsOpen()) {
        skillPanel_.Update(dt, input, context);
        if (skillPanel_.CloseRequested()) activeTab_ = HomeTab::None;
    } else if (questPanel_.IsOpen()) {
        questPanel_.Update(dt, input, context);
        if (questPanel_.StartRequested()) {
            startQuest_ = true;
            questPanel_.Close();
            activeTab_ = HomeTab::None;
        }
        if (questPanel_.CloseRequested()) activeTab_ = HomeTab::None;
    } else if (upgradePanel_.IsOpen()) {
        upgradePanel_.Update(dt, input, context);
        player_.Setup(context.player);
        player_.FullHeal();
        if (upgradePanel_.CloseRequested()) activeTab_ = HomeTab::None;
    } else if (settingsPanel_.IsOpen()) {
        settingsPanel_.Update(dt, input, context);
        camera_.SetShakeEnabled(context.settings.screenShake);
        if (settingsPanel_.CloseRequested()) activeTab_ = HomeTab::None;
    }

    // --- タブボタン ----------------------------------------------------------
    const bool panelOpen = AnyPanelOpen();
    for (int i = 0; i < static_cast<int>(tabButtons_.size()); ++i) {
        tabButtons_[static_cast<size_t>(i)].SetSelected(kTabs[i].tab == activeTab_);
        if (tabButtons_[static_cast<size_t>(i)].Update(input, dt) && !panelOpen) {
            OpenTab(kTabs[i].tab, context);
        }
    }

    // 数字キーでもタブを開ける
    if (!panelOpen) {
        const GameAction keys[4] = { GameAction::Skill1, GameAction::Skill2,
                                     GameAction::Skill3, GameAction::Skill4 };
        for (int i = 0; i < 4 && i < kTabCount; ++i) {
            if (input.Pressed(keys[i])) OpenTab(kTabs[i].tab, context);
        }
        // 5 番目以降は生キーで受け付ける
        if (kTabCount >= 5 && input.KeyPressed(KEY_INPUT_5)) OpenTab(kTabs[4].tab, context);
        if (input.Pressed(GameAction::Menu)) OpenTab(HomeTab::Settings, context);
    }

    // --- フィールド ----------------------------------------------------------
    stage_.Update(dt);
    player_.Update(dt, stage_, combat_, input, !panelOpen);
    combat_.Update(dt);

    // ゲートに触れたらクエスト選択を開く
    if (!panelOpen && !startQuest_ && player_.pos.x > kGateX - 60.0f) {
        OpenTab(HomeTab::Quest, context);
        const float z = player_.z;
        player_.PlaceAt(Vec2(kGateX - 160.0f, stage_.GroundYAt(z)));
        player_.z = z;
    }

    const float lookAhead = static_cast<float>(player_.facing) * 120.0f;
    camera_.SetTarget(Vec2(player_.pos.x + lookAhead, kScreenH * 0.5f));
    camera_.Update(dt);

    // --- 出撃 ---------------------------------------------------------------
    if (startQuest_ && !manager.IsTransitioning()) {
        startQuest_ = false;
        SaveSystem::Save(context);   // 出撃前にも保存しておく
        manager.RequestChange(SceneId::Quest);
    }
}

void HomeScene::Draw(GameContext& context)
{
    DrawField(context);

    combat_.DrawBehindActors(camera_);
    player_.Draw(camera_);
    combat_.DrawFrontOfActors(camera_, context.settings.showDamageNumbers);

    DrawFieldGuide(context);
    DrawPlayerSummary(context);
    DrawTabBar(context);

    // --- パネル -------------------------------------------------------------
    if (AnyPanelOpen()) {
        ui::DrawDimOverlay(160);
        equipPanel_.Draw(context);
        skillPanel_.Draw(context);
        questPanel_.Draw(context);
        upgradePanel_.Draw(context);
        settingsPanel_.Draw();
    }

    if (saveNoticeTimer_ > 0.0f) {
        const int alpha = static_cast<int>(math::Clamp(saveNoticeTimer_ / 0.8f, 0.0f, 1.0f) * 255.0f);
        draw::TextAlpha(FontSize::Small, kScreenW - 30.0f, kScreenH - kTabBarHeight - 40.0f,
                        palette::kHp, "セーブしました", alpha, draw::TextAlign::Right);
    }

    if (context.settings.showFps) {
        draw::Text(FontSize::Small, kScreenW - 24.0f, 24.0f, palette::kTextDim,
                   str::Format("FPS %.1f", GetFPS()), draw::TextAlign::Right);
    }
}

void HomeScene::DrawField(const GameContext& context)
{
    (void)context;
    stage_.DrawBackground(camera_);

    // 出撃ゲート
    const Rect gate = Rect::FromCenter(kGateX, stage_.GroundY() - 190.0f, 220.0f, 380.0f);
    if (camera_.IsVisible(gate, 200.0f)) {
        const Rect screen = camera_.WorldToScreen(gate);
        const float pulse = 0.8f + 0.2f * std::sin(time_ * 2.4f);

        draw::Glow(screen.CenterX(), screen.CenterY(), 140.0f * pulse, palette::kAccent, 120, 5);
        draw::StrokeRect(screen, palette::kAccent, 4.0f, 220);
        draw::FillRect(screen, palette::kAccent.Scaled(0.25f), 120);
        draw::Text(FontSize::Medium, screen.CenterX(), screen.top - 60.0f, palette::kAccent,
                   "出撃ゲート", draw::TextAlign::Center);
        draw::Text(FontSize::Small, screen.CenterX(), screen.top - 24.0f, palette::kTextDim,
                   "近づくとクエスト選択", draw::TextAlign::Center);
    }

    // 案内看板
    const Rect board = Rect::FromCenter(700.0f, stage_.GroundY() - 120.0f, 240.0f, 150.0f);
    if (camera_.IsVisible(board, 200.0f)) {
        const Rect screen = camera_.WorldToScreen(board);
        draw::FillRect(screen, ColorRGB(58, 46, 38), 255);
        draw::StrokeRect(screen, ColorRGB(120, 96, 70), 3.0f, 255);
        draw::Text(FontSize::Small, screen.CenterX(), screen.top + 16.0f, palette::kText,
                   "THE ECLIPSE", draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, screen.CenterX(), screen.top + 54.0f, palette::kTextDim,
                   "拠点", draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, screen.CenterX(), screen.top + 84.0f, palette::kTextDim,
                   "→ 東へ", draw::TextAlign::Center);
        draw::Line(screen.CenterX() - 8.0f, screen.bottom, screen.CenterX() - 8.0f, screen.bottom + 60.0f,
                   ColorRGB(90, 72, 54), 8.0f, 255);
    }
}

void HomeScene::DrawFieldGuide(const GameContext& context) const
{
    (void)context;
    // 操作ヒント
    const Rect hint = Rect::FromXYWH(kScreenW - 420.0f, kScreenH - kTabBarHeight - 132.0f,
                                     392.0f, 116.0f);
    draw::ChamferRect(hint, 10.0f, palette::kPanelDark, 190);
    draw::StrokeRect(hint, palette::kBorder.Scaled(0.6f), 1.0f, 150);
    draw::Text(FontSize::Tiny, hint.left + 16.0f, hint.top + 12.0f, palette::kAccent, "操作");
    draw::Text(FontSize::Tiny, hint.left + 16.0f, hint.top + 38.0f, palette::kTextDim,
               "移動 : A / D   奥行き : W / S   ジャンプ : SPACE");
    draw::Text(FontSize::Tiny, hint.left + 16.0f, hint.top + 62.0f, palette::kTextDim,
               "攻撃 : 左クリック   ガード : 右クリック長押し");
    draw::Text(FontSize::Tiny, hint.left + 16.0f, hint.top + 86.0f, palette::kTextDim,
               "回避 : SHIFT   パリィ : ガード中に左クリック   タブ : 1〜5");
}

void HomeScene::DrawPlayerSummary(const GameContext& context) const
{
    const PlayerData& data = context.player;
    const Rect panel = Rect::FromXYWH(28.0f, 24.0f, 560.0f, 150.0f);

    draw::ChamferRect(panel, 16.0f, palette::kPanel, 225);
    draw::StrokeRect(panel, palette::kBorder, 2.0f, 200);

    draw::Text(FontSize::Medium, panel.left + 24.0f, panel.top + 14.0f, palette::kText, data.Name());
    draw::Text(FontSize::Medium, panel.right - 24.0f, panel.top + 14.0f, palette::kAccent,
               str::Format("Lv %d", data.Level()), draw::TextAlign::Right);

    // EXP バー
    const Rect expBar = Rect::FromXYWH(panel.left + 24.0f, panel.top + 62.0f, panel.Width() - 48.0f, 16.0f);
    const float expRatio = (data.ExpToNext() > 0)
                         ? static_cast<float>(data.Exp()) / static_cast<float>(data.ExpToNext())
                         : 0.0f;
    draw::Bar(expBar, expRatio, palette::kExp, palette::kPanelDark);
    draw::Text(FontSize::Tiny, expBar.left, expBar.bottom + 6.0f, palette::kTextDim,
               str::Format("EXP  %d / %d", data.Exp(), data.ExpToNext()));

    draw::Text(FontSize::Small, panel.right - 24.0f, panel.top + 96.0f, palette::kAccentWarm,
               str::Format("%s col", str::Comma(data.GetInventory().Col()).c_str()),
               draw::TextAlign::Right);
    draw::Text(FontSize::Small, panel.left + 24.0f, panel.top + 112.0f, palette::kTextDim,
               str::Format("戦力 %d ／ 武器 %s", data.Power(),
                           WeaponTypeName(data.CurrentWeaponType())));
}

void HomeScene::DrawTabBar(const GameContext& context) const
{
    const Rect bar = Rect::FromXYWH(0.0f, kScreenH - kTabBarHeight, kScreenW, kTabBarHeight);
    draw::GradientRectV(bar, palette::kPanel.Scaled(0.9f), palette::kPanelDark, 240, 10);
    draw::Line(bar.left, bar.top, bar.right, bar.top, palette::kAccent, 2.0f, 200);

    for (int i = 0; i < static_cast<int>(tabButtons_.size()); ++i) {
        tabButtons_[static_cast<size_t>(i)].Draw();

        // 未使用のスキルポイントがあればスキルタブに知らせる
        if (kTabs[i].tab != HomeTab::Skill) continue;
        const int points = context.player.SkillPoints();
        if (points <= 0) continue;

        const Rect rect = tabButtons_[static_cast<size_t>(i)].GetRect();
        const float pulse = 0.75f + 0.25f * std::sin(time_ * 4.0f);
        const float cx = rect.right - 16.0f;
        const float cy = rect.top + 4.0f;
        draw::Glow(cx, cy, 22.0f * pulse, palette::kExp, 120, 3);
        draw::Circle(cx, cy, 17.0f, palette::kExp, true, 1.0f, 255);
        draw::Circle(cx, cy, 17.0f, palette::kBlack, false, 2.0f, 255);
        draw::Text(FontSize::Tiny, cx, cy - 9.0f, palette::kBlack,
                   str::Format("%d", points), draw::TextAlign::Center);
    }
}

} // namespace ecl
