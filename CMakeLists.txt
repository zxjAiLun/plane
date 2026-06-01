cmake_minimum_required(VERSION 3.22)
project(PlaneShooter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

FetchContent_MakeAvailable(SFML)

add_executable(PlaneShooter
    src/main.cpp
    src/Game.cpp
    src/GameWorld.cpp
    src/Input.cpp
    src/Player.cpp
    src/Renderer.cpp
    src/Projectile.cpp
    src/Weapon.cpp
    src/ExperienceOrb.cpp
    src/Timer.cpp
    src/Enemy.cpp
    src/EnemySpawner.cpp
    src/Collision.cpp
)

target_include_directories(PlaneShooter PRIVATE include)
target_link_libraries(PlaneShooter PRIVATE SFML::Graphics SFML::Window SFML::System)
