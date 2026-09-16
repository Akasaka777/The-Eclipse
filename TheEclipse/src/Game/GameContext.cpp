#include "Game/GameContext.h"

namespace ecl {

void QuestResult::Reset()
{
    *this = QuestResult();
}

const char* QuestResult::Rank() const
{
    if (!cleared) return "D";

    // 被ダメージが少なく、素早くクリアしたほど高評価
    int score = 0;
    if (damageTaken == 0) score += 3;
    else if (damageTaken < 400) score += 2;
    else if (damageTaken < 1200) score += 1;

    if (clearTime < 60.0f) score += 3;
    else if (clearTime < 120.0f) score += 2;
    else if (clearTime < 210.0f) score += 1;

    if (maxCombo >= 30) score += 2;
    else if (maxCombo >= 15) score += 1;

    if (score >= 7) return "S";
    if (score >= 5) return "A";
    if (score >= 3) return "B";
    return "C";
}

} // namespace ecl
