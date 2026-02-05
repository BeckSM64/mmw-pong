#pragma once
#include <cstdint>

struct PaddleInput {
    uint32_t id;
    float dy;
};

struct PaddleState {
    uint32_t id;
    float y;
};

struct BallState {
    float x;
    float y;
    float vx;
    float vy;
};

struct GameState {
    PaddleState left;
    PaddleState right;
    BallState ball;
};

