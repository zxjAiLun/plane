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
    src/Timer.cpp
    src/Enemy.cpp
    src/EnemySpawner.cpp
    src/Collision.cpp
    src/SaveService.cpp
)

target_include_directories(PlaneShooter PRIVATE include)
target_link_libraries(PlaneShooter PRIVATE SFML::Graphics SFML::Window SFML::System)

# Pure-logic unit tests: gameplay data/math plus lightweight timer/spawner code.
add_executable(arpg_logic_tests
    tests/arpg_logic_tests.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/EnemySpawner.cpp
    src/Timer.cpp
    src/SaveService.cpp
)
target_include_directories(arpg_logic_tests PRIVATE include)
# Player.cpp needs no SFML; keep the test binary free of graphics deps.
enable_testing()
add_test(NAME arpg_logic_tests COMMAND arpg_logic_tests)

add_executable(arpg_save_tests
    tests/save_logic_tests.cpp
    src/Player.cpp
    src/SaveService.cpp
)
target_include_directories(arpg_save_tests PRIVATE include)
add_test(NAME arpg_save_tests COMMAND arpg_save_tests)

add_executable(arpg_world_tests
    tests/game_world_logic_tests.cpp
    src/GameWorld.cpp
    src/Input.cpp
    src/Player.cpp
    src/Projectile.cpp
    src/Timer.cpp
    src/Enemy.cpp
    src/EnemySpawner.cpp
    src/Collision.cpp
    src/SaveService.cpp
)
target_include_directories(arpg_world_tests PRIVATE include)
target_link_libraries(arpg_world_tests PRIVATE SFML::Window SFML::System)
add_test(NAME arpg_world_tests COMMAND arpg_world_tests)

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET PlaneShooter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:PlaneShooter>/assets"
    )
endif()
