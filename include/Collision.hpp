#pragma once

#include "Vector2.hpp"

namespace Collision {
    bool circleCircle(
        const Vector2& aPosition, float aRadius,
        const Vector2& bPosition, float bRadius
    );
}
