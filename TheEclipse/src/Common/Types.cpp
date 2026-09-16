#include "Common/Types.h"
#include "Common/MathUtil.h"

#include <cmath>

namespace ecl {

float Vec2::Length() const
{
    return std::sqrt(x * x + y * y);
}

float Vec2::LengthSq() const
{
    return x * x + y * y;
}

Vec2 Vec2::Normalized() const
{
    const float len = Length();
    if (len <= 0.0001f) return Vec2(0.0f, 0.0f);
    return Vec2(x / len, y / len);
}

float Distance(const Vec2& a, const Vec2& b)
{
    return (a - b).Length();
}

Rect Rect::FromXYWH(float x, float y, float w, float h)
{
    return Rect(x, y, x + w, y + h);
}

Rect Rect::FromCenter(float cx, float cy, float w, float h)
{
    return Rect(cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f);
}

Rect Rect::FromFoot(float footX, float footY, float halfWidth, float height)
{
    return Rect(footX - halfWidth, footY - height, footX + halfWidth, footY);
}

bool Rect::Intersects(const Rect& o) const
{
    return !(right < o.left || o.right < left || bottom < o.top || o.bottom < top);
}

bool Rect::Contains(float px, float py) const
{
    return px >= left && px <= right && py >= top && py <= bottom;
}

Rect Rect::Offset(float dx, float dy) const
{
    return Rect(left + dx, top + dy, right + dx, bottom + dy);
}

Rect Rect::Expanded(float amount) const
{
    return Rect(left - amount, top - amount, right + amount, bottom + amount);
}

ColorRGB ColorRGB::Scaled(float f) const
{
    return ColorRGB(math::ClampInt(static_cast<int>(r * f), 0, 255),
                    math::ClampInt(static_cast<int>(g * f), 0, 255),
                    math::ClampInt(static_cast<int>(b * f), 0, 255));
}

ColorRGB ColorRGB::Lerp(const ColorRGB& a, const ColorRGB& b, float t)
{
    t = math::Clamp(t, 0.0f, 1.0f);
    return ColorRGB(static_cast<int>(a.r + (b.r - a.r) * t),
                    static_cast<int>(a.g + (b.g - a.g) * t),
                    static_cast<int>(a.b + (b.b - a.b) * t));
}

} // namespace ecl
