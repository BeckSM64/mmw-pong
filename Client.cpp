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
        PaddleInput in{ myId, dy };
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

/* ---------- 16:9 VIEW HANDLING ---------- */
static void updateLetterboxView(sf::RenderWindow& window, sf::View& view) {
    const float targetRatio = 16.f / 9.f;
    sf::Vector2u size = window.getSize();
    float windowRatio = static_cast<float>(size.x) / size.y;

    float vpX = 0.f, vpY = 0.f, vpW = 1.f, vpH = 1.f;

    if (windowRatio > targetRatio) {
        // Pillarbox
        vpW = targetRatio / windowRatio;
        vpX = (1.f - vpW) * 0.5f;
    } else {
        // Letterbox
        vpH = windowRatio / targetRatio;
        vpY = (1.f - vpH) * 0.5f;
    }

    view.setViewport(sf::FloatRect(vpX, vpY, vpW, vpH));
}

int main(int argc, char** argv) {
    uint32_t myId = (argc > 1) ? std::atoi(argv[1]) : 1;
    Network net(myId);

    sf::RenderWindow window(
        sf::VideoMode(1280, 720),
        "MMW Pong",
        sf::Style::Default
    );
    window.setFramerateLimit(60);

    sf::View view(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
    updateLetterboxView(window, view);
    window.setView(view);

    const float paddleSpeed = 600.f;
    const float sendInterval = 1.f / 20.f;

    sf::Clock frameClock;
    sf::Clock sendClock;

    float localLeftY  = 540.f;
    float localRightY = 540.f;

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();

            if (e.type == sf::Event::Resized) {
                updateLetterboxView(window, view);
                window.setView(view);
            }
        }

        float dt = frameClock.restart().asSeconds();

        float localDy = 0.f;
        float netDy   = 0.f;

        if (myId == 1) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
                localDy -= paddleSpeed * dt;
                netDy   -= paddleSpeed * sendInterval;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                localDy += paddleSpeed * dt;
                netDy   += paddleSpeed * sendInterval;
            }

            localLeftY += localDy;
            if (localLeftY < 80.f)   localLeftY = 80.f;
            if (localLeftY > 1000.f) localLeftY = 1000.f;
        } else {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                localDy -= paddleSpeed * dt;
                netDy   -= paddleSpeed * sendInterval;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                localDy += paddleSpeed * dt;
                netDy   += paddleSpeed * sendInterval;
            }

            localRightY += localDy;
            if (localRightY < 80.f)   localRightY = 80.f;
            if (localRightY > 1000.f) localRightY = 1000.f;
        }

        if (netDy != 0.f && sendClock.getElapsedTime().asSeconds() >= sendInterval) {
            net.sendInput(netDy);
            sendClock.restart();
        }

        GameState s = net.getState();

        if (myId == 1)
            localRightY = s.right.y;
        else
            localLeftY = s.left.y;

        window.clear(sf::Color::Black);

        sf::RectangleShape paddle({20.f, 160.f});
        paddle.setOrigin(10.f, 80.f);

        paddle.setPosition(40.f, localLeftY);
        window.draw(paddle);

        paddle.setPosition(1880.f, localRightY);
        window.draw(paddle);

        sf::CircleShape ball(10.f);
        ball.setOrigin(10.f, 10.f);
        ball.setPosition(s.ball.x, s.ball.y);
        window.draw(ball);

        window.display();
    }
}
