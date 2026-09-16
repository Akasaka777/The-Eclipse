//==============================================================================
// DxInclude.h : DxLib.h をインクルードする唯一の窓口
//   windows.h の min/max マクロが std::min/std::max と衝突するため
//   必ずこのヘッダ経由で取り込むこと。
//==============================================================================
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "DxLib.h"
