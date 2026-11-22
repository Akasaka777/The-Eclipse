#include "Scenes/TitleScene.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Core/Version.h"
#include "Game/GameContext.h"
#include "Game/SaveData.h"
#include "Graphics/DrawUtil.h"

#include <cmath>
#include <string>
#include <vector>

namespace ecl {

namespace {
constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);

//------------------------------------------------------------------------------
// タイトル画面：Ars Gladii（剣の技）
//   夜明けの空、地平の朝日を背に丘へ突き立つ大きな剣。周りの丘には大小の剣が刺さり、火の粉が舞う。
//------------------------------------------------------------------------------
const ColorRGB kTitleGold(232, 194, 118);     // 装飾・サブタイトル・ボタンの縁
const ColorRGB kTitleSteel(206, 218, 232);    // 刃の明るい面
const ColorRGB kTitleInk(10, 9, 16);          // 縁取り
constexpr float kHorizonY = 640.0f;           // 地平（朝日の中心）
constexpr float kSwordX = kScreenW * 0.5f;    // 大きな剣の位置
constexpr float kGuardY = 248.0f;             // 鍔の高さ
constexpr float kSwordTipY = 704.0f;          // 刃先（丘に刺さっている）

const char* kTitleSubtitle = "― 剣は技となり、技は道となる ―";
const char* kTitleCaption = "T H E   A R T   O F   T H E   S W O R D";

float Hash(int i)
{
    const float v = std::sin(static_cast<float>(i) * 12.9898f) * 43758.5453f;
    return v - std::floor(v);
}

// 不透明な上下グラデーション（帯の境目を整数にそろえる）
void SkyBand(float top, float bottom, const ColorRGB& from, const ColorRGB& to, int bands)
{
    for (int i = 0; i < bands; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(bands - 1);
        const float y0 = std::floor(top + (bottom - top) * static_cast<float>(i) / static_cast<float>(bands));
        const float y1 = (i == bands - 1) ? bottom
                                          : std::floor(top + (bottom - top) * static_cast<float>(i + 1) / static_cast<float>(bands));
        draw::FillRect(Rect(0.0f, y0, kScreenW, y1), ColorRGB::Lerp(from, to, t), 255);
    }
}

// なだらかな丘の影（左から右へ、上端を波打たせて塗る）
void Hills(float baseY, float amplitude, float waves, float phase, const ColorRGB& color)
{
    const int steps = 48;
    for (int i = 0; i < steps; ++i) {
        const float x0 = kScreenW * static_cast<float>(i) / static_cast<float>(steps);
        const float x1 = kScreenW * static_cast<float>(i + 1) / static_cast<float>(steps);
        auto height = [&](float x) {
            const float u = x / kScreenW;
            return baseY - amplitude * (0.6f * std::sin(u * waves * 6.2831853f + phase)
                                        + 0.4f * std::sin(u * waves * 2.3f * 6.2831853f + phase * 1.7f));
        };
        const float y0 = height(x0);
        const float y1 = height(x1);
        draw::Triangle(Vec2(x0, y0), Vec2(x1, y1), Vec2(x0, kScreenH), color, true, 255);
        draw::Triangle(Vec2(x1, y1), Vec2(x1, kScreenH), Vec2(x0, kScreenH), color, true, 255);
        // 継ぎ目が透けないよう、帯の下は四角で埋める
        draw::FillRect(Rect(x0, math::MaxF(y0, y1), x1 + 1.0f, kScreenH), color, 255);
    }
}

// 丘に刺さった剣の影（angle は傾き。rim は朝日の当たる縁）
void PlantedSword(float x, float groundY, float length, float angleDeg, const ColorRGB& color, const ColorRGB& rim,
                  int rimAlpha)
{
    const float a = math::DegToRad(angleDeg);
    const Vec2 up(std::sin(a), -std::cos(a));      // 刃先から柄頭へ
    const Vec2 side(-up.y, up.x);
    const Vec2 tip(x, groundY + length * 0.12f);   // 刃先は少し地面に埋まる
    const Vec2 guard = tip + up * (length * 0.70f);
    const Vec2 pommel = tip + up * length;
    const float bw = length * 0.045f;
    draw::Triangle(tip - side * (bw * 0.2f), guard - side * bw, guard + side * bw, color, true, 255);
    draw::Triangle(tip - side * (bw * 0.2f), tip + side * (bw * 0.2f), guard + side * bw, color, true, 255);
    draw::Line(guard.x - side.x * bw * 3.2f, guard.y - side.y * bw * 3.2f, guard.x + side.x * bw * 3.2f,
               guard.y + side.y * bw * 3.2f, color, math::MaxF(2.0f, bw * 0.9f), 255);
    draw::Line(guard.x, guard.y, pommel.x, pommel.y, color, math::MaxF(2.0f, bw * 0.8f), 255);
    draw::Circle(pommel.x, pommel.y, math::MaxF(2.0f, bw * 0.9f), color, true, 1.0f, 255);
    // 朝日の側（右）の縁だけ細く光らせる
    draw::Line(tip.x + side.x * bw * 0.2f, tip.y + side.y * bw * 0.2f, guard.x + side.x * bw, guard.y + side.y * bw, rim,
               math::MaxF(1.0f, bw * 0.25f), rimAlpha);
}

// 中央の大きな剣（丘に突き立つ。刃に光が上下に走る）
void GreatSword(float time)
{
    const float cx = kSwordX;
    const float guardY = kGuardY;
    const float bladeTop = guardY + 14.0f;
    const float tipY = kSwordTipY;
    const float halfW = 30.0f;

    // 刃（縁取り → 暗い面 → 明るい面 → 樋 → 刃の縁の光）
    const std::vector<Vec2> blade = { Vec2(cx - halfW, bladeTop), Vec2(cx + halfW, bladeTop), Vec2(cx + halfW * 0.82f, tipY - 110.0f),
                                      Vec2(cx, tipY), Vec2(cx - halfW * 0.82f, tipY - 110.0f) };
    auto fan = [](const std::vector<Vec2>& pts, const ColorRGB& color) {
        Vec2 c(0.0f, 0.0f);
        for (const Vec2& p : pts) c += p;
        c *= 1.0f / static_cast<float>(pts.size());
        for (int pass = 0; pass < 2; ++pass) {
            for (size_t i = 0; i < pts.size(); ++i) draw::Triangle(c, pts[i], pts[(i + 1) % pts.size()], color, true, 255);
        }
    };
    std::vector<Vec2> outline;
    for (const Vec2& p : blade) outline.push_back(Vec2(cx + (p.x - cx) * 1.12f, p.y + ((p.y > tipY - 1.0f) ? 6.0f : -2.0f)));
    fan(outline, kTitleInk);
    fan(blade, ColorRGB(112, 124, 146));
    fan({ Vec2(cx - halfW, bladeTop), Vec2(cx, bladeTop), Vec2(cx, tipY), Vec2(cx - halfW * 0.82f, tipY - 110.0f) },
        kTitleSteel);
    draw::Line(cx - 2.0f, bladeTop + 10.0f, cx - 2.0f, tipY - 160.0f, ColorRGB(132, 144, 166), 6.0f, 255);   // 樋
    draw::Line(cx - halfW + 2.0f, bladeTop + 4.0f, cx - halfW * 0.82f + 2.0f, tipY - 112.0f, palette::kWhite, 2.0f, 200);
    // 刃を上下に走る光
    const float sweep = std::fmod(time * 0.25f, 1.0f);
    const float gy = bladeTop + (tipY - 140.0f - bladeTop) * (sweep < 0.5f ? sweep * 2.0f : 2.0f - sweep * 2.0f);
    draw::Glow(cx - 8.0f, gy, 46.0f, palette::kWhite, 120, 4);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
    DrawLineAA(cx - 30.0f, gy, cx + 14.0f, gy, draw::ToDx(palette::kWhite), 2.0f);
    DrawLineAA(cx - 8.0f, gy - 22.0f, cx - 8.0f, gy + 22.0f, draw::ToDx(palette::kWhite), 2.0f);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 鍔（金。両端は丸め、中央に宝玉）
    const ColorRGB gold = kTitleGold;
    draw::FillRect(Rect(cx - 140.0f, guardY - 3.0f, cx + 140.0f, guardY + 21.0f), kTitleInk, 255);
    draw::FillRect(Rect(cx - 136.0f, guardY, cx + 136.0f, guardY + 18.0f), gold.Scaled(0.8f), 255);
    draw::FillRect(Rect(cx - 136.0f, guardY, cx + 136.0f, guardY + 7.0f), gold, 255);
    for (int s = -1; s <= 1; s += 2) {
        const float ex = cx + static_cast<float>(s) * 140.0f;
        draw::Circle(ex, guardY + 9.0f, 17.0f, kTitleInk, true, 1.0f, 255);
        draw::Circle(ex, guardY + 9.0f, 14.0f, gold, true, 1.0f, 255);
        draw::Circle(ex - 3.0f, guardY + 5.0f, 5.0f, ColorRGB(255, 240, 200), true, 1.0f, 230);
    }
    draw::Circle(cx, guardY + 9.0f, 22.0f, kTitleInk, true, 1.0f, 255);
    draw::Circle(cx, guardY + 9.0f, 19.0f, gold, true, 1.0f, 255);
    draw::Circle(cx, guardY + 9.0f, 11.0f, ColorRGB(196, 40, 56), true, 1.0f, 255);
    draw::Glow(cx, guardY + 9.0f, 28.0f, ColorRGB(255, 80, 90), 90 + static_cast<int>(40.0f * std::sin(time * 2.0f)), 3);
    draw::Circle(cx - 3.0f, guardY + 5.0f, 4.0f, ColorRGB(255, 200, 200), true, 1.0f, 230);

    // 握り（革を巻いた柄）と柄頭
    const float gripTop = guardY - 104.0f;
    draw::FillRect(Rect(cx - 16.0f, gripTop, cx + 16.0f, guardY), kTitleInk, 255);
    draw::FillRect(Rect(cx - 13.0f, gripTop, cx + 13.0f, guardY), ColorRGB(64, 40, 30), 255);
    for (int i = 0; i < 7; ++i) {
        const float y = gripTop + 8.0f + static_cast<float>(i) * 14.0f;
        draw::Line(cx - 13.0f, y + 6.0f, cx + 13.0f, y - 2.0f, ColorRGB(104, 70, 50), 3.0f, 255);
    }
    draw::Circle(cx, gripTop - 14.0f, 21.0f, kTitleInk, true, 1.0f, 255);
    draw::Circle(cx, gripTop - 14.0f, 18.0f, gold, true, 1.0f, 255);
    draw::Circle(cx - 5.0f, gripTop - 19.0f, 6.0f, ColorRGB(255, 240, 200), true, 1.0f, 220);
}

// 画面の端を暗くする（境目は整数にそろえて半透明を重ねない）
void Vignette()
{
    const int bands = 12;
    const float edge = 260.0f;
    for (int i = 0; i < bands; ++i) {
        const int alpha = static_cast<int>(150.0f * (1.0f - static_cast<float>(i) / static_cast<float>(bands)));
        const float x0 = std::floor(edge * static_cast<float>(i) / static_cast<float>(bands));
        const float x1 = std::floor(edge * static_cast<float>(i + 1) / static_cast<float>(bands));
        draw::FillRect(Rect(x0, 0.0f, x1, kScreenH), palette::kBlack, alpha);
        draw::FillRect(Rect(kScreenW - x1, 0.0f, kScreenW - x0, kScreenH), palette::kBlack, alpha);
    }
}

// 背景一式
void DrawTitleBackground(float time)
{
    // 空：上は夜の名残の藍、地平へ向かって朝焼けの赤銅色
    SkyBand(0.0f, 430.0f, ColorRGB(6, 8, 20), ColorRGB(34, 28, 62), 24);
    SkyBand(430.0f, kHorizonY + 40.0f, ColorRGB(34, 28, 62), ColorRGB(150, 84, 62), 24);

    // 消えかけの星（上ほど多く、地平に近いほど薄い）
    for (int i = 0; i < 110; ++i) {
        const float x = Hash(i * 3 + 1) * kScreenW;
        const float y = Hash(i * 3 + 2) * 400.0f;
        const float twinkle = 0.45f + 0.55f * std::sin(time * 1.3f + static_cast<float>(i));
        const float fade = 1.0f - y / 420.0f;
        draw::Circle(x, y, 0.8f + Hash(i * 3 + 3) * 1.4f, palette::kWhite, true, 1.0f,
                     static_cast<int>(150.0f * twinkle * fade));
    }

    // 地平の朝日と、放たれる光の筋
    draw::Glow(kSwordX, kHorizonY, 560.0f, ColorRGB(255, 176, 104), 130, 14);
    draw::Glow(kSwordX, kHorizonY, 220.0f, ColorRGB(255, 226, 170), 170, 9);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 9);
    for (int i = 0; i < 15; ++i) {
        const float a = math::DegToRad(-172.0f + static_cast<float>(i) * (164.0f / 14.0f) + std::sin(time * 0.3f) * 1.5f);
        const float len = 1500.0f;
        const float spread = math::DegToRad((i % 2 == 0) ? 1.8f : 1.0f);
        DrawTriangleAA(kSwordX, kHorizonY, kSwordX + std::cos(a - spread) * len, kHorizonY + std::sin(a - spread) * len,
                       kSwordX + std::cos(a + spread) * len, kHorizonY + std::sin(a + spread) * len,
                       draw::ToDx(ColorRGB(255, 214, 150)), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 遠い山並み（朝もやで明るめ）
    for (int i = 0; i < 9; ++i) {
        const float x = Hash(i * 5 + 40) * kScreenW * 1.2f - kScreenW * 0.1f;
        const float h = 90.0f + Hash(i * 5 + 41) * 120.0f;
        const float w = 260.0f + Hash(i * 5 + 42) * 260.0f;
        draw::Triangle(Vec2(x - w, kHorizonY + 30.0f), Vec2(x + w, kHorizonY + 30.0f), Vec2(x, kHorizonY + 30.0f - h),
                       ColorRGB(70, 52, 72), true, 255);
    }

    // 奥の丘と、そこに刺さる小さな剣
    Hills(678.0f, 22.0f, 1.6f, 0.8f, ColorRGB(42, 30, 48));
    for (int i = 0; i < 16; ++i) {
        const float x = Hash(i * 7 + 70) * kScreenW;
        if (std::fabs(x - kSwordX) < 190.0f) continue;   // 中央の大きな剣のまわりは空ける
        PlantedSword(x, 676.0f + Hash(i * 7 + 71) * 20.0f, 60.0f + Hash(i * 7 + 72) * 30.0f,
                     (Hash(i * 7 + 73) - 0.5f) * 40.0f, ColorRGB(34, 24, 40), ColorRGB(255, 196, 130), 110);
    }

    // 中央：大きな剣と、刺さっている岩の丘
    GreatSword(time);
    draw::Circle(kSwordX, kSwordTipY + 150.0f, 210.0f, ColorRGB(26, 19, 30), true, 1.0f, 255);
    draw::Circle(kSwordX - 60.0f, kSwordTipY + 40.0f, 70.0f, ColorRGB(30, 22, 34), true, 1.0f, 255);
    draw::Circle(kSwordX + 70.0f, kSwordTipY + 50.0f, 60.0f, ColorRGB(28, 20, 32), true, 1.0f, 255);
    // 岩の上の縁に朝日が当たる（下側は手前の丘に隠れる）
    draw::Circle(kSwordX, kSwordTipY + 150.0f, 210.0f, ColorRGB(255, 190, 120), false, 2.0f, 80);

    // 手前の丘と、大きく見える剣（ボタンのある中央は空ける）
    Hills(800.0f, 36.0f, 1.1f, 2.3f, ColorRGB(18, 13, 22));
    for (int i = 0; i < 12; ++i) {
        const float x = Hash(i * 11 + 120) * kScreenW;
        if (std::fabs(x - kSwordX) < 330.0f) continue;
        PlantedSword(x, 800.0f + Hash(i * 11 + 121) * 60.0f, 130.0f + Hash(i * 11 + 122) * 90.0f,
                     (Hash(i * 11 + 123) - 0.5f) * 46.0f, ColorRGB(14, 10, 18), ColorRGB(255, 200, 140), 150);
    }

    // 舞い上がる火の粉
    for (int i = 0; i < 46; ++i) {
        const float speed = 26.0f + Hash(i * 13 + 200) * 40.0f;
        const float life = std::fmod(time * speed / 700.0f + Hash(i * 13 + 201), 1.0f);
        const float x = Hash(i * 13 + 202) * kScreenW + std::sin(time * 0.7f + static_cast<float>(i)) * 24.0f;
        const float y = kScreenH - 60.0f - life * 760.0f;
        const int alpha = static_cast<int>(200.0f * std::sin(life * 3.14159f));
        draw::Circle(x, y, 1.4f + Hash(i * 13 + 203) * 1.8f, ColorRGB(255, 196, 120), true, 1.0f, alpha);
    }

    Vignette();
}
} // namespace

TitleScene::TitleScene()
{
    startButton_ = ui::Button(Rect::FromXYWH(kScreenW * 0.5f - 200.0f, 700.0f, 400.0f, 78.0f),
                              "ゲームを始める", FontSize::Medium);
    exitButton_ = ui::Button(Rect::FromXYWH(kScreenW * 0.5f - 200.0f, 800.0f, 400.0f, 62.0f),
                             "終了", FontSize::Normal);
    // 金と鋼の色合いに合わせる
    startButton_.SetAccent(kTitleGold);
    exitButton_.SetAccent(kTitleGold);
}

void TitleScene::OnEnter(GameContext& context)
{
    time_ = 0.0f;
    exitRequested_ = false;

    // セーブデータがあれば読み込む
    if (!loadAttempted_) {
        loadAttempted_ = true;
        if (SaveSystem::Exists() && SaveSystem::Load(context)) {
            hasSaveData_ = true;
        } else if (context.player.GetInventory().Items().empty()) {
            // 初回は名前と武器種を訊いてから初期装備を配る
            starterPending_ = true;
        }

        // セーブデータが無い初回だけ、プレイヤー名を訊く
        if (!hasSaveData_) {
            nameAsked_ = true;
            namePanel_.SetTitle("プレイヤー名を入力");
            namePanel_.SetCancelEnabled(false);   // 初回は必ず決めてもらう
            namePanel_.Open("");                  // 初回は空から入力してもらう
        }
    }
}

void TitleScene::Update(float dt, GameContext& context, SceneManager& manager)
{
    time_ += dt;
    const Input& input = Input::Instance();

    // --- 初回の名前入力 --------------------------------------------------------
    if (namePanel_.IsOpen()) {
        namePanel_.Update(dt, input);
        if (namePanel_.Confirmed()) {
            context.player.SetName(namePanel_.Result());
            // 名前を決めたら、続けて使う武器種を選んでもらう
            if (starterPending_) weaponPanel_.Open();
        }
        return;
    }

    // --- 初回の武器種選択 ------------------------------------------------------
    if (weaponPanel_.IsOpen()) {
        weaponPanel_.Update(dt, input);
        if (weaponPanel_.Confirmed()) {
            chosenWeapon_ = weaponPanel_.Result();
            // 選んだ武器種の武器だけを配る
            context.player.SetupNewGame(chosenWeapon_);
            starterPending_ = false;
            weaponChosen_ = true;
        }
        return;
    }

    if (manager.IsTransitioning()) return;

    if (startButton_.Update(input, dt) || input.Pressed(GameAction::Confirm)) {
        manager.RequestChange(SceneId::Home);
    }
    if (exitButton_.Update(input, dt)) {
        exitRequested_ = true;
        context.quitRequested = true;
    }
}

void TitleScene::Draw(GameContext& context)
{

    // --- 背景（夜明けの空・朝日・丘に突き立つ剣）-------------------------------------
    DrawTitleBackground(time_);

    // --- タイトル ------------------------------------------------------------
    const float cx = kScreenW * 0.5f;
    // 文字の後ろをうっすら暗くして、剣や朝日の上でも読みやすくする（半透明の楕円を重ねる）
    for (int i = 0; i < 12; ++i) {
        const float k = 1.0f - 0.07f * static_cast<float>(i);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 11);
        DrawOvalAA(cx, 522.0f, 470.0f * k, 120.0f * k, 48, draw::ToDx(palette::kBlack), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    draw::Text(FontSize::Tiny, cx, 444.0f, kTitleGold.Scaled(0.9f), kTitleCaption, draw::TextAlign::Center);
    draw::TextShadow(FontSize::Title, cx, 470.0f, ColorRGB(250, 244, 230), config::kGameTitle,
                     draw::TextAlign::Center);
    draw::Text(FontSize::Normal, cx, 580.0f, kTitleGold, kTitleSubtitle, draw::TextAlign::Center);

    // 装飾ライン（中央で切れ、両端と中央にひし形）
    const float lineY = 562.0f;
    draw::Line(cx - 360.0f, lineY, cx - 24.0f, lineY, kTitleGold, 2.0f, 200);
    draw::Line(cx + 24.0f, lineY, cx + 360.0f, lineY, kTitleGold, 2.0f, 200);
    for (float dx : { -368.0f, 0.0f, 368.0f }) {
        const float d = (dx == 0.0f) ? 9.0f : 6.0f;
        draw::Triangle(Vec2(cx + dx - d, lineY), Vec2(cx + dx, lineY - d), Vec2(cx + dx + d, lineY), kTitleGold, true, 255);
        draw::Triangle(Vec2(cx + dx - d, lineY), Vec2(cx + dx, lineY + d), Vec2(cx + dx + d, lineY), kTitleGold, true, 255);
    }

    startButton_.Draw();
    exitButton_.Draw();

    if (hasSaveData_) {
        draw::Text(FontSize::Small, kScreenW * 0.5f, 660.0f, palette::kHp,
                   str::Format("セーブデータを読み込みました（%s / Lv %d）",
                               context.player.Name().c_str(), context.player.Level()),
                   draw::TextAlign::Center);
    } else if (nameAsked_ && !namePanel_.IsOpen()) {
        const std::string welcome = weaponChosen_
            ? str::Format("ようこそ、%s さん（%s 使い）", context.player.Name().c_str(),
                          WeaponTypeName(chosenWeapon_))
            : str::Format("ようこそ、%s さん", context.player.Name().c_str());
        draw::Text(FontSize::Small, kScreenW * 0.5f, 660.0f, palette::kAccent, welcome,
                   draw::TextAlign::Center);
    }
    draw::Text(FontSize::Tiny, kScreenW * 0.5f, kScreenH - 60.0f, palette::kTextDim,
               "ENTER / クリックで開始   横スクロール 2D アクション RPG プロトタイプ",
               draw::TextAlign::Center);
    // バージョン（右下）
    draw::Text(FontSize::Small, kScreenW - 24.0f, kScreenH - 60.0f, palette::kTextDim,
               version::kVersionLabel, draw::TextAlign::Right);
    draw::Text(FontSize::Tiny, kScreenW - 24.0f, kScreenH - 32.0f, palette::kTextDisabled,
               str::Format("%s 版   DxLib / C++17", version::kVersionDate), draw::TextAlign::Right);

    // 名前入力・武器種選択は最前面に重ねる
    namePanel_.Draw();
    weaponPanel_.Draw();
}

} // namespace ecl
