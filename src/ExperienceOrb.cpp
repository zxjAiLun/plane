#include "ExperienceOrb.hpp"
#include "Config.hpp"

ExperienceOrb::ExperienceOrb(const Vector2& position, int value)
    : position_(position)
    , radius_(Config::ExpOrbRadius)
    , value_(value)
    , collected_(false) {
}

void ExperienceOrb::update(float /*dt*/) {
}

const Vector2& ExperienceOrb::position() const { return position_; }
float ExperienceOrb::radius() const { return radius_; }
int ExperienceOrb::value() const { return value_; }

bool ExperienceOrb::isCollected() const { return collected_; }
void ExperienceOrb::collect() { collected_ = true; }
