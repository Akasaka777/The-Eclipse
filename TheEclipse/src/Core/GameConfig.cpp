#include "Core/GameConfig.h"

namespace ecl {
namespace palette {

const ColorRGB kBackground(8, 10, 18);
const ColorRGB kPanel(18, 22, 34);
const ColorRGB kPanelLight(34, 42, 62);
const ColorRGB kPanelDark(12, 15, 24);
const ColorRGB kBorder(72, 118, 168);
const ColorRGB kAccent(64, 206, 255);
const ColorRGB kAccentWarm(255, 172, 64);
const ColorRGB kText(232, 240, 252);
const ColorRGB kTextDim(150, 166, 192);
const ColorRGB kTextDisabled(92, 102, 122);
const ColorRGB kHp(88, 222, 130);
const ColorRGB kHpMid(178, 226, 96);
const ColorRGB kHpLow(255, 214, 92);
const ColorRGB kHpCritical(238, 78, 78);
const ColorRGB kHpLoss(226, 92, 92);
const ColorRGB kMp(82, 168, 255);
const ColorRGB kExp(255, 214, 92);
const ColorRGB kBossHp(232, 72, 82);
const ColorRGB kDanger(255, 86, 86);
const ColorRGB kCritical(255, 226, 96);
const ColorRGB kBlack(0, 0, 0);
const ColorRGB kWhite(255, 255, 255);

ColorRGB HpColor(float ratio)
{
    // 100〜76% 緑 / 75〜51% 黄緑 / 50〜26% 黄 / 25〜0% 赤
    if (ratio > 0.75f) return kHp;
    if (ratio > 0.50f) return kHpMid;
    if (ratio > 0.25f) return kHpLow;
    return kHpCritical;
}

} // namespace palette
} // namespace ecl
