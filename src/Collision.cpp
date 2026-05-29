#include "Collision.hpp"

bool Collision::circleCircle(
    const Vector2& aPosition, float aRadius,
    const Vector2& bPosition, float bRadius
) {
    const float dx = aPosition.x - bPosition.x;
    const float dy = aPosition.y - bPosition.y;
    const float distanceSquared = dx * dx + dy * dy;

    const float radiusSum = aRadius + bRadius;
    return distanceSquared <= radiusSum * radiusSum;
}
