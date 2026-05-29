#pragma once

class Timer {
public:
    explicit Timer(float duration = 0.0f);

    void update(float dt);
    void reset();

    bool isReady() const;
    void setDuration(float duration);

private:
    float duration_;
    float elapsed_;
};
