#include "Game.hpp"
#include "Config.hpp"

#include <optional>

Game::Game()
    : window_(sf::VideoMode({Config::WindowWidth, Config::WindowHeight}), "PlaneShooter")
    , renderer_(window_) {
}

void Game::run() {
    sf::Clock clock;

    while (window_.isOpen()) {
        float dt = clock.restart().asSeconds();

        processInput();
        update(dt);
        render();
    }
}

void Game::processInput() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        }
        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            input_.handleKeyPressed(key->code);
            if (key->code == sf::Keyboard::Key::Escape) {
                window_.close();
            }
        }
        if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
            input_.handleKeyReleased(key->code);
        }
        if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
            input_.handleMouseMoved(mouse->position);
        }
        if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            input_.handleMousePressed(mouse->button, mouse->position);
        }
    }
}

void Game::update(float dt) {
    world_.update(dt, input_);
}

void Game::render() {
    renderer_.render(world_);
}
