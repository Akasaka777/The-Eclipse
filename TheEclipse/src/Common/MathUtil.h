//==============================================================================
// MathUtil.h : 数学ユーティリティと乱数
//==============================================================================
#pragma once

namespace ecl {
namespace math {

constexpr float kPi = 3.14159265358979323846f;

float Clamp(float value, float lo, float hi);
int   ClampInt(int value, int lo, int hi);
float Lerp(float a, float b, float t);
// cur を target に delta だけ近づける
float Approach(float cur, float target, float delta);
float Sign(float v);
float Abs(float v);
float MinF(float a, float b);
float MaxF(float a, float b);
int   MinI(int a, int b);
int   MaxI(int a, int b);
float DegToRad(float deg);
// 0→1 を滑らかに補間
float SmoothStep(float t);
// dt に依存しない指数減衰係数
float DampFactor(float rate, float dt);

void  SeedRandom(unsigned int seed);
int   RandInt(int minInclusive, int maxInclusive);
float RandFloat(float lo, float hi);
bool  RandChance(float probability01);

} // namespace math
} // namespace ecl
