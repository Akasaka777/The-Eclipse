//==============================================================================
// GameContext.h : シーン間で共有する状態
//==============================================================================
#pragma once

#include "Core/GameConfig.h"
#include "Game/Equipment.h"
#include "Game/PlayerData.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// クエスト結果（リザルト画面で使用）
//------------------------------------------------------------------------------
struct QuestResult
{
    bool        cleared = false;
    bool        retired = false;
    int         questId = 0;
    std::string questName;
    float       clearTime = 0.0f;
    int         floorsCleared = 0;
    int         floorCount = 0;
    int         enemiesDefeated = 0;
    int         maxCombo = 0;
    int         totalDamage = 0;
    int         damageTaken = 0;
    int         expGained = 0;
    int         colGained = 0;
    int         materialGained = 0;
    int         levelsGained = 0;
    bool        firstClear = false;
    std::vector<EquipmentItem> drops;

    void Reset();
    // S / A / B / C 評価
    const char* Rank() const;
};

class GameContext
{
public:
    PlayerData   player;
    GameSettings settings;
    QuestResult  lastResult;
    int          selectedQuestId = 1;
    float        globalTime = 0.0f;
    // タイトルの「終了」などから立てる終了要求
    bool         quitRequested = false;
};

} // namespace ecl
