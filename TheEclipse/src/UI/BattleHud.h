//==============================================================================
// BattleHud.h : 戦闘中の HUD
//   左上 : キャラアイコン＋HP/MP  上部中央 : ボス HP  右下 : スキルバー
//   上部右 : MENU ボタン
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/Boss.h"
#include "Game/Player.h"
#include "Game/PlayerData.h"
#include "Game/SwordSkill.h"
#include "UI/UIWidgets.h"

#include <string>

namespace ecl {
namespace ui {

//------------------------------------------------------------------------------
// HUD に表示する進行情報
//------------------------------------------------------------------------------
struct HudInfo
{
    std::string floorName;
    int   floorIndex = 1;
    int   floorCount = 1;
    int   enemiesRemaining = 0;
    float questTime = 0.0f;
    int   combo = 0;
    float comboTimer = 0.0f;
    bool  showFps = false;
};

class BattleHud
{
public:
    BattleHud();

    void Reset();
    void Update(float dt, const Player& player, const Boss* boss, const Input& input);
    void Draw(const Player& player, const PlayerData& data, const Boss* boss, const HudInfo& info) const;

    // クリック結果（Update 後に参照）
    int  ClickedSkillIndex() const { return clickedSkill_; }

private:
    void DrawPlayerStatus(const Player& player, const PlayerData& data) const;
    void DrawBossStatus(const Boss& boss) const;
    void DrawSkillBar(const Player& player) const;
    void DrawFloorInfo(const HudInfo& info) const;
    void DrawCombo(const HudInfo& info) const;

    Rect   skillRects_[kSkillSlotCount];
    float  hpDelay_ = 1.0f;
    float  bossHpDelay_ = 1.0f;
    int    clickedSkill_ = -1;
    float  time_ = 0.0f;
};

} // namespace ui
} // namespace ecl
