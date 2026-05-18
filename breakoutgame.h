#pragma once
#include <iostream>
#include <raylib.h>
#include <vector>
#include <memory>
#include <enet/enet.h>
// 关卡数据结构
struct LevelData {
    int rows;
    int cols;
    std::vector<std::vector<int>> brickLayout;
};

// 道具类型枚举
enum class PowerUpType {
    PADDLE_EXTEND,
    MULTI_BALL,
    SLOW_BALL
};

// 游戏状态枚举
enum class GameStateEnum {
    START,
    PLAYING,
    GAME_OVER,
    WIN,
    LOAD_MENU   // <-- 就加这一行！
};

// 网络同步的游戏状态结构体（必须和客户端一致）
struct NetworkGameState {
    float ballX, ballY;
    float ballSpeedX, ballSpeedY;
    float paddle1X, paddle2X;
    int score;
    int life;
};

// 挡板类
class Paddle {
private:
    Rectangle rect;
    float originalWidth;
    float extendTimer;
public:
    Paddle(float x, float y, float width, float height) 
        : rect({x, y, width, height}), originalWidth(width), extendTimer(0.0f) {}
    
    void Update() {
        float mouseX = GetMouseX();
        rect.x = mouseX - rect.width / 2.0f;
        if (rect.x < 0) rect.x = 0;
        if (rect.x + rect.width > GetScreenWidth()) rect.x = GetScreenWidth() - rect.width;
        
        if (extendTimer > 0) extendTimer -= GetFrameTime();
        else rect.width = originalWidth;
    }

    void Draw() const { DrawRectangleRec(rect, BLUE); }
    void Extend(float extra, float time) { rect.width = originalWidth + extra; extendTimer = time; }
    Rectangle GetRect() const { return rect; }
    void SetX(float x) { rect.x = x; }
};

// 球类
class Ball {
private:
    Vector2 position;
    Vector2 speed;
    float radius;
    float slowTimer;
    float slowFactor;
public:
    Ball(float x, float y, float speedX, float speedY, float radius)
        : position({x, y}), speed({speedX, speedY}), radius(radius), slowTimer(0.0f), slowFactor(1.0f) {}

    void Update() {
        if (slowTimer > 0) {
            slowTimer -= GetFrameTime();
            if (slowTimer <= 0) slowFactor = 1.0f;
        }
        position.x += speed.x * slowFactor * GetFrameTime();
        position.y += speed.y * slowFactor * GetFrameTime();
    }

    void Draw() const { DrawCircleV(position, radius, WHITE); }
    void BounceX() { speed.x *= -1; }
    void BounceY() { speed.y *= -1; }
    void Slow(float factor, float time) { slowFactor = factor; slowTimer = time; }

    Vector2 GetPosition() const { return position; }
    float GetRadius() const { return radius; }
    Vector2 GetSpeed() const { return speed; }
    void SetPosition(Vector2 pos) { position = pos; }
    void SetSpeed(Vector2 spd) { speed = spd; }
    Ball* Clone() const { return new Ball(position.x, position.y, speed.x, speed.y, radius); }
};

// 砖块类
class Brick {
private:
    Rectangle rect;
    Color color;
    bool destroyed;
public:
    Brick(float x, float y, float width, float height, Color color)
        : rect({x, y, width, height}), color(color), destroyed(false) {}

    void Draw() const { if (!destroyed) DrawRectangleRec(rect, color); }
    void Destroy() { destroyed = true; }
    bool IsDestroyed() const { return destroyed; }
    Rectangle GetRect() const { return rect; }
    Color GetColor() const { return color; }
};

// 道具类
class PowerUp {
private:
    Vector2 position;
    PowerUpType type;
    bool active;
    float speed;
public:
    PowerUp(float x, float y, PowerUpType type)
        : position({x, y}), type(type), active(true), speed(150.0f) {}

    void Update() {
        if (active) position.y += speed * GetFrameTime();
        if (position.y > GetScreenHeight()) active = false;
    }

    void Draw() const {
        if (active) {
            switch (type) {
                case PowerUpType::PADDLE_EXTEND: DrawCircleV(position, 10, GREEN); break;
                case PowerUpType::MULTI_BALL: DrawCircleV(position, 10, YELLOW); break;
                case PowerUpType::SLOW_BALL: DrawCircleV(position, 10, RED); break;
            }
        }
    }

    bool CheckCollision(Rectangle paddleRect) {
        return active && CheckCollisionCircleRec(position, 10, paddleRect);
    }

    void Activate() { active = false; }
    bool IsActive() const { return active; }
    PowerUpType GetType() const { return type; }
    Vector2 GetPosition() const { return position; }
};

// 粒子类
class Particle {
private:
    Vector2 position;
    Vector2 velocity;
    Color color;
    float lifetime;
    float maxLifetime;
public:
    Particle(Vector2 pos, Vector2 vel, Color col, float life)
        : position(pos), velocity(vel), color(col), lifetime(life), maxLifetime(life) {}

    void Update() {
        lifetime -= GetFrameTime();
        position.x += velocity.x * GetFrameTime();
        position.y += velocity.y * GetFrameTime();
        color.a = (unsigned char)(255 * (lifetime / maxLifetime));
    }

    void Draw() const { if (lifetime > 0) DrawPixelV(position, color); }
    bool IsDead() const { return lifetime <= 0; }
};

// 核心游戏类（主机专用，和你的实现完全匹配）
class BreakoutGame {
private:
    // 游戏核心成员（和你原来的变量名一致）
    std::unique_ptr<Paddle> paddle1; // 主机控制的下方挡板
    std::unique_ptr<Paddle> paddle2; // 客户端控制的上方挡板
    std::vector<Ball*> balls;
    std::vector<Brick*> bricks;
    std::vector<PowerUp> powerUps;
    std::vector<Particle> particles;
    int score;
    int life;
    GameStateEnum gameState;
    bool isBallPaused;

     // 👇👇👇 在这里加关卡相关变量 👇👇👇
    int nowLevel;                // 当前关卡
    float curBallSpeedRate;      // 球速倍率
    float curPaddleSpeedRate;    // 挡板速度倍率
    void LoadLevelData(int lid); // 加载关卡函数
    // 网络成员（主机用）
    ENetHost* enetHost;
    ENetPeer* clientPeer;

    // 碰撞检测（原有逻辑）
    void HandleCollisions();

    // 网络同步函数
    void SendGameState();
    void ReceiveClientInput();

public:
    // 构造/析构
    BreakoutGame();
    ~BreakoutGame();

    // 主循环
    void UpdateGame();
    void DrawGame();
    void Run();
};

void saveLevel(const std::string& filename, const LevelData& level);



// 全局存档变量（游戏启动时加载，游戏中保存）
struct SaveData {
    int score;
    int life;
    LevelData level;
    Vector2 ballPos;   // 新增：球的位置
    Vector2 ballSpeed; // 新增：球的速度
    int now_level;
};

extern SaveData gameSave;
extern bool hasSaveFile;
LevelData loadLevel(const std::string& filename);
