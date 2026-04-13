#ifndef BREAKOUTGAME_H
#define BREAKOUTGAME_H

#include <raylib.h>
#include <vector>
#include "powerup.hpp"
#include "particle.hpp"

// 新增：游戏状态枚举
enum class GameState {
    START,      // 新增：起始界面
    PLAYING,    // 游戏中
    GAME_OVER,  // 游戏失败
    WIN         // 游戏获胜
};

// 前置声明
class Ball;
class Paddle;
class Brick;

// 核心游戏类
class BreakoutGame {
private:
    // 窗口参数
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    // 游戏对象
    Paddle* paddle;
    std::vector<Ball*> balls;
    std::vector<Brick*> bricks;
    std::vector<Particle> particles;
    std::vector<PowerUp> powerUps;
    
    // 游戏状态（新增GameState）
    int score;
    int life;
    GameState gameState; // 替换原isGameOver
    bool isBallPaused;

    // 私有方法
    void InitGame();          // 初始化游戏
    void UpdateGame();        // 更新逻辑
    void DrawGame();          // 绘制画面
    void SpawnParticles(Vector2 pos, Color color); // 生成粒子
    void SpawnPowerUp(Vector2 pos);                // 生成道具
    void HandleCollisions();  // 处理碰撞
    void Cleanup();           // 清理资源
    bool CheckWin();          // 新增：判断是否获胜

public:
    BreakoutGame();           // 构造函数
    ~BreakoutGame();          // 析构函数
    void Run();               // 游戏主循环
};

// Ball 类（补充GetSpeed方法）
class Ball {
private:
    Vector2 position;
    Vector2 speed;
    float radius;
    float slowTimer;
    float slowFactor;

public:
    Ball(float x, float y, float sx, float sy, float r);
    void Update();
    void Draw() const;
    void BounceX();
    void BounceY();
    void Slow(float factor, float time);
    Ball* Clone() const;
    
    Vector2 GetPosition() const { return position; }
    float GetRadius() const { return radius; }
    void SetPosition(Vector2 pos) { position = pos; }
    void SetSpeed(Vector2 sp) { speed = sp; }
    Vector2 GetSpeed() const { return speed; } // 新增：补充GetSpeed接口
};

// Paddle 类
class Paddle {
private:
    float originalWidth;
    float extendTimer;
public: 
    Rectangle rect;
    Paddle(float x, float y, float w, float h);
    void Update();
    void Draw() const;
    void Extend(float extra, float time);
    
    Rectangle GetRect() const { return rect; }
};

// Brick 类（修正构造函数声明）
class Brick {
private:
    Rectangle rect;
    bool destroyed;
    Color color; // 新增：砖块颜色

public:
    Brick(float x, float y, float w, float h, Color col); // 带颜色的构造函数
    void Draw() const;
    void Destroy() { destroyed = true; }
    bool IsDestroyed() const { return destroyed; }
    Rectangle GetRect() const { return rect; }
    Vector2 GetPosition() const { return {rect.x + rect.width/2, rect.y + rect.height/2}; }
    Color GetColor() const { return color; } // 新增：获取颜色
};

#endif