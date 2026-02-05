#include <SFML/Graphics.hpp>
#include <mutex>
#include <cstdlib>
#include "MMW.h"
#include "GameState.h"

class Network {
public:
    explicit Network(uint32_t id) : myId(id) {
        instance = this;
        mmw_set_log_level(MMW_LOG_LEVEL_OFF);
        mmw_initialize("204.1.6.186", 20666);
        mmw_create_publisher("input");
        mmw_create_subscriber_raw("state", stateCallbackStatic);
    }

    ~Network() { mmw_cleanup(); }

    void sendInput(float dy) {
        PaddleInput in{myId, dy};
        mmw_publish_raw("input", &in, sizeof(in), MMW_BEST_EFFORT);
    }

    GameState getState() {
        std::lock_guard<std::mutex> lk(mtx);
        return state;
    }

private:
    static Network* instance;
    uint32_t myId;
    GameState state{};
    std::mutex mtx;

    static void stateCallbackStatic(const char*, void* data) {
        instance->stateCallback(data);
    }

    void stateCallback(void* data) {
        std::lock_guard<std::mutex> lk(mtx);
        state = *reinterpret_cast<GameState*>(data);
    }
};

Network* Network::instance = nullptr;

int main(int argc, char** argv) {
    uint32_t myId = (argc > 1) ? std::atoi(argv[1]) : 1;
    Network net(myId);

    sf::RenderWindow window(sf::VideoMode(1280, 720), "MMW Pong");
    sf::View view(sf::FloatRect(0, 0, 1920, 1080));
    window.setView(view);
    window.setFramerateLimit(60);

    sf::Clock sendClock;
    const float sendInterval = 1.f / 20.f;
    const float paddleSpeed = 600.f;

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();
        }

        float dy = 0.f;

        // Player-specific keys
        if (myId == 1) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
                dy -= paddleSpeed * sendInterval;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                dy += paddleSpeed * sendInterval;
        } else if (myId == 2) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                dy -= paddleSpeed * sendInterval;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                dy += paddleSpeed * sendInterval;
        }

        if (dy != 0.f && sendClock.getElapsedTime().asSeconds() >= sendInterval) {
            net.sendInput(dy);
            sendClock.restart();
        }

        GameState s = net.getState();

        window.clear(sf::Color::Black);

        sf::RectangleShape paddle({20.f, 160.f});
        paddle.setOrigin(10.f, 80.f);

        paddle.setPosition(40.f, s.left.y);
        window.draw(paddle);

        paddle.setPosition(1880.f, s.right.y);
        window.draw(paddle);

        sf::CircleShape ball(10.f);
        ball.setOrigin(10.f, 10.f);
        ball.setPosition(s.ball.x, s.ball.y);
        window.draw(ball);

        window.display();
    }
}

