#include "Common/MathUtil.h"

#include <cmath>
#include <random>

namespace ecl {
namespace math {

namespace {
std::mt19937& Engine()
{
    static std::mt19937 engine(20240101u);
    return engine;
}
} // namespace

float Clamp(float value, float lo, float hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

int ClampInt(int value, int lo, int hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float Approach(float cur, float target, float delta)
{
    if (cur < target) {
        cur += delta;
        if (cur > target) cur = target;
    } else if (cur > target) {
        cur -= delta;
        if (cur < target) cur = target;
    }
    return cur;
}

float Sign(float v)
{
    if (v > 0.0f) return 1.0f;
    if (v < 0.0f) return -1.0f;
    return 0.0f;
}

float Abs(float v) { return v < 0.0f ? -v : v; }
float MinF(float a, float b) { return a < b ? a : b; }
float MaxF(float a, float b) { return a > b ? a : b; }
int   MinI(int a, int b) { return a < b ? a : b; }
int   MaxI(int a, int b) { return a > b ? a : b; }

float DegToRad(float deg)
{
    return deg * kPi / 180.0f;
}

float SmoothStep(float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float DampFactor(float rate, float dt)
{
    return 1.0f - std::exp(-rate * dt);
}

void SeedRandom(unsigned int seed)
{
    Engine().seed(seed);
}

int RandInt(int minInclusive, int maxInclusive)
{
    if (maxInclusive <= minInclusive) return minInclusive;
    std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
    return dist(Engine());
}

float RandFloat(float lo, float hi)
{
    if (hi <= lo) return lo;
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(Engine());
}

bool RandChance(float probability01)
{
    if (probability01 <= 0.0f) return false;
    if (probability01 >= 1.0f) return true;
    return RandFloat(0.0f, 1.0f) < probability01;
}

} // namespace math
} // namespace ecl
