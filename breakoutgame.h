#ifndef BREAKOUT_GAME_H
#define BREAKOUT_GAME_H

#include <raylib.h>
#include <vector>
#include <string>

class Brick {
private:
    Rectangle rect;
    bool active;
    Color color;
    int points;

public:
    Brick(float x, float y, float width, float height, Color c, int p);
    int CheckHit(const Vector2& ballPos, float ballRadius);
    void Draw() const;
    bool IsActive() const;
};

class BreakoutGame {
private:
    const int screenWidth = 800;
    const int screenHeight = 450;

    Rectangle player;
    Vector2 ballPos;
    Vector2 ballSpeed;
    float ballRadius;
    bool gameStarted;

    std::vector<Brick> bricks;
    int score;
    int lives;
    bool gameOver;

    void InitBricks();
    void ResetBall();
    void ResetGame();
    void Update();
    void Draw();

public:
    BreakoutGame();
    void Run();
};

#endif