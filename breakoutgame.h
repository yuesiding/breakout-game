// #ifndef BREAKOUT_GAME_H
// #define BREAKOUT_GAME_H

// #include <raylib.h>
// #include <vector>
// #include <string>

// class Brick {
// private:
//     Rectangle rect;
//     bool active;
//     Color color;
//     int points;

// public:
//     Brick(float x, float y, float width, float height, Color c, int p);
//     int CheckHit(const Vector2& ballPos, float ballRadius);
//     void Draw() const;
//     bool IsActive() const;
// };

// class BreakoutGame {
// private:
//     const int screenWidth = 800;
//     const int screenHeight = 450;

//     Rectangle player;
//     Vector2 ballPos;
//     Vector2 ballSpeed;
//     float ballRadius;
//     bool gameStarted;

//     std::vector<Brick> bricks;
//     int score;
//     int lives;
//     bool gameOver;

//     void InitBricks();
//     void ResetBall();
//     void ResetGame();
//     void Update();
//     void Draw();

// public:
//     BreakoutGame();
//     void Run();
// };

// #endif

#ifndef BREAKOUT_GAME_H
#define BREAKOUT_GAME_H

#include <raylib.h>
#include <vector>
#include <string>
// 新增：JSON库头文件（需安装nlohmann/json）
#include <nlohmann/json.hpp>
// 新增：文件流头文件
#include <fstream>
// 新增：标准输入输出
#include <iostream>

// 命名空间别名（简化JSON代码）
using json = nlohmann::json;

// 新增：游戏状态枚举（替代bool瘟疫）
enum class GameState {
    MENU,         // 主菜单
    PLAYING,      // 游戏中
    PAUSED,       // 暂停
    GAMEOVER,     // 游戏结束（失败）
    VICTORY,      // 胜利
    LEADERBOARD   // 排行榜界面
};

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
    // ------------ 替换原有的硬编码 + 新增配置变量 ------------
    int screenWidth;
    int screenHeight;
    std::string windowTitle;
    
    float ballRadius;
    float ballGravity;
    float ballMaxSpeed;
    float ballBounceForce;
    
    float paddleWidth;
    float paddleHeight;
    float paddleSpeed;
    float paddleBoostSpeed;
    
    int bricksRows;
    int bricksCols;
    float brickWidth;
    float brickHeight;
    
    int initialLives;
    int scorePerBrick;
    float timeMultiplierDecay;

    // ------------ 原有的游戏变量 ------------
    Rectangle player;
    Vector2 ballPos;
    Vector2 ballSpeed;
    bool gameStarted;

    std::vector<Brick> bricks;
    int score;
    int lives;
    // ------------ 替换原有的 bool 状态 ------------
    GameState currentState;  // 替代 gameOver、paused 等 bool

    // ------------ 新增私有方法 ------------
    void LoadConfig(const std::string& configPath);  // 加载JSON配置
    void InitBricks();
    void ResetBall();
    void ResetGame();
    void Update();
    void Draw();
    // 新增：检查砖块是否全清（用于胜利判断）
    bool AllBricksCleared() const;

public:
    BreakoutGame();
    void Run();
};

#endif