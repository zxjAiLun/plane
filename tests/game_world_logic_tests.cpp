#include <cstdint>
#include <iostream>
#include <string>

#include "GameWorld.hpp"
#include "Input.hpp"

namespace {

int failures = 0;

void expect(bool condition, const std::string& label) {
    if (condition) {
        std::cout << "  PASS  " << label << '\n';
    } else {
        ++failures;
        std::cout << "  FAIL  " << label << '\n';
    }
}

std::string enemySignature(const GameWorld& world) {
    std::string signature;
    for (const auto& enemy : world.enemies()) {
        signature += std::to_string(static_cast<int>(enemy.type())) + ":"
            + std::to_string(static_cast<int>(enemy.eliteModifier())) + ":"
            + std::to_string(enemy.hp()) + ":"
            + std::to_string(enemy.position().x) + ":"
            + std::to_string(enemy.position().y) + ";";
    }
    return signature;
}

void advanceIntoTheField(GameWorld& world, Input& input) {
    input.handleKeyPressed(sf::Keyboard::Key::D);
    for (int frame = 0; frame < 60; ++frame) {
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::D);
}

void settleEnemies(GameWorld& world, Input& input) {
    for (int frame = 0; frame < 35; ++frame) {
        world.update(0.05f, input);
    }
}

} // namespace

int main() {
    std::cout << "ARPG GameWorld RNG tests\n";

    GameWorld first(4401);
    GameWorld second(4401);
    GameWorld different(4402);
    Input firstInput;
    Input secondInput;
    Input differentInput;

    advanceIntoTheField(first, firstInput);
    advanceIntoTheField(second, secondInput);
    advanceIntoTheField(different, differentInput);
    settleEnemies(first, firstInput);
    settleEnemies(second, secondInput);
    settleEnemies(different, differentInput);

    expect(first.state() == GameState::Playing
            && second.state() == GameState::Playing
            && different.state() == GameState::Playing,
        "identical short runs remain in the Playing state");
    expect(!first.enemies().empty() && !second.enemies().empty(),
        "field simulation produces enemies through the real spawn path");
    expect(first.runSeed() == 4401 && second.runSeed() == 4401
            && different.runSeed() == 4402,
        "GameWorld retains the configured run seed");
    expect(enemySignature(first) == enemySignature(second),
        "same GameWorld seed reproduces enemy encounter state");
    expect(enemySignature(first) != enemySignature(different),
        "different GameWorld seeds change enemy encounter state");

    const std::uint64_t originalSeed = first.runSeed();
    first.reset();
    expect(first.runSeed() != originalSeed, "reset derives a new run seed");
    first.reset(4401);
    expect(first.runSeed() == 4401, "explicit reset restores a requested run seed");

    std::cout << "Passed: " << (failures == 0 ? 7 : 7 - failures)
        << "  Failed: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
