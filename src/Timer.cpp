#include "Timer.hpp"

Timer::Timer(float duration)
    : duration_(duration)
    , elapsed_(0.0f) {
}

void Timer::update(float dt) {
    elapsed_ += dt;
}

void Timer::reset() {
    elapsed_ = 0.0f;
}

bool Timer::isReady() const {
    return elapsed_ >= duration_;
}

void Timer::setDuration(float duration) {
    duration_ = duration;
}
