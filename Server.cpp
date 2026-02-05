#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <cmath>
#include "MMW.h"
#include "GameState.h"

static GameState game;
static std::mutex gameMutex;

void inputCallback(const char*, void* data) {
    PaddleInput* in = reinterpret_cast<PaddleInput*>(data);

    std::lock_guard<std::mutex> lk(gameMutex);

    PaddleState* p =
        (in->id == 1) ? &game.left :
        (in->id == 2) ? &game.right :
        nullptr;

    if (!p) return;

    p->y += in->dy;
    if (p->y < 80.f)   p->y = 80.f;
    if (p->y > 1000.f) p->y = 1000.f;
}

int main() {
    mmw_set_log_level(MMW_LOG_LEVEL_INFO);
    mmw_initialize("127.0.0.1", 20666);

    mmw_create_subscriber_raw("input", inputCallback);
    mmw_create_publisher("state");

    game.left  = {1, 540.f};
    game.right = {2, 540.f};
    game.ball  = {960.f, 540.f, 350.f, 220.f};

    auto last = std::chrono::high_resolution_clock::now();

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;

        {
            std::lock_guard<std::mutex> lk(gameMutex);
            BallState& b = game.ball;

            b.x += b.vx * dt;
            b.y += b.vy * dt;

            if (b.y < 0.f || b.y > 1080.f) b.vy *= -1.f;

            if (b.x < 60.f && std::fabs(b.y - game.left.y) < 90.f)
                b.vx = std::fabs(b.vx);

            if (b.x > 1860.f && std::fabs(b.y - game.right.y) < 90.f)
                b.vx = -std::fabs(b.vx);

            if (b.x < 0.f || b.x > 1920.f) {
                b.x = 960.f;
                b.y = 540.f;
                b.vx *= -1.f;
            }

            mmw_publish_raw("state", &game, sizeof(game), MMW_BEST_EFFORT);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 20 Hz
    }
}

