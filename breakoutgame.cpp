#include "breakoutgame.h"
#include <iostream>

// ---------------- Brick 实现 ----------------
Brick::Brick(float x, float y, float width, float height, Color c, int p) {
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    active = true;
    color = c;
    points = p;
}

int Brick::CheckHit(const Vector2& ballPos, float ballRadius) {
    if (active && CheckCollisionCircleRec(ballPos, ballRadius, rect)) {
        active = false;
        return points;
    }
    return 0;
}

void Brick::Draw() const {
    if (active) {
        DrawRectangleRec(rect, color);
        DrawRectangleLinesEx(rect, 1, DARKGRAY);
    }
}

bool Brick::IsActive() const {
    return active;
}

// ---------------- BreakoutGame 实现 ----------------
BreakoutGame::BreakoutGame() {
    // 第一步：加载JSON配置（替换硬编码）
    LoadConfig("config.json");
    
    // 第二步：用配置初始化窗口
    InitWindow(screenWidth, screenHeight, windowTitle.c_str());
    
    // 第三步：用配置初始化玩家（挡板）
    player = { (float)screenWidth/2 - paddleWidth/2, (float)screenHeight - 50, paddleWidth, paddleHeight };
    
    // 第四步：初始化其他变量
    ballSpeed = { 5.0f, -5.0f };
    score = 0;
    lives = initialLives;
    gameStarted = false;
    currentState = GameState::MENU;  // 初始状态：主菜单

    InitBricks();
    ResetBall();

    SetTargetFPS(60);
}

// 新增：加载JSON配置文件
void BreakoutGame::LoadConfig(const std::string& configPath) {
    try {
        std::ifstream f(configPath);
        if (!f.is_open()) {
            std::cerr << "Error: Could not open config file: " << configPath << std::endl;
            // 回退到硬编码默认值
            screenWidth = 800;
            screenHeight = 450;
            windowTitle = "Raylib C++ 打砖块（生命值版）";
            
            ballRadius = 8.0f;
            ballGravity = 0.08f;
            ballMaxSpeed = 15.0f;
            ballBounceForce = 0.5f;
            
            paddleWidth = 100.0f;
            paddleHeight = 15.0f;
            paddleSpeed = 18.0f;
            paddleBoostSpeed = 28.0f;
            
            bricksRows = 5;
            bricksCols = 10;
            brickWidth = 70.0f;
            brickHeight = 25.0f;
            
            initialLives = 5;
            scorePerBrick = 10;
            timeMultiplierDecay = 0.05f;
            return;
        }
        
        json config;
        f >> config;
        
        // 读取窗口配置
        screenWidth = config["window"]["width"];
        screenHeight = config["window"]["height"];
        windowTitle = config["window"]["title"].get<std::string>();
        
        // 读取球配置
        ballRadius = config["ball"]["radius"];
        ballGravity = config["ball"]["gravity"];
        ballMaxSpeed = config["ball"]["maxSpeed"];
        ballBounceForce = config["ball"]["bounceForce"];
        
        // 读取挡板配置
        paddleWidth = config["paddle"]["width"];
        paddleHeight = config["paddle"]["height"];
        paddleSpeed = config["paddle"]["speed"];
        paddleBoostSpeed = config["paddle"]["boostSpeed"];
        
        // 读取砖块配置
        bricksRows = config["bricks"]["rows"];
        bricksCols = config["bricks"]["cols"];
        brickWidth = config["bricks"]["width"];
        brickHeight = config["bricks"]["height"];
        
        // 读取游戏配置
        initialLives = config["game"]["initialLives"];
        scorePerBrick = config["game"]["scorePerBrick"];
        timeMultiplierDecay = config["game"]["timeMultiplierDecay"];
        
        std::cout << "Config loaded successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        // 回退到默认值
        screenWidth = 800;
        screenHeight = 450;
        windowTitle = "Raylib C++ 打砖块（生命值版）";
    }
}

// 新增：检查所有砖块是否被清除（胜利条件）
bool BreakoutGame::AllBricksCleared() const {
    for (const auto& brick : bricks) {
        if (brick.IsActive()) {
            return false;
        }
    }
    return true;
}

void BreakoutGame::InitBricks() {
    bricks.clear();
    for (int i = 0; i < bricksRows; i++) {
        Color rowColor;
        int rowPoints = (bricksRows - i) * scorePerBrick;
        
        switch (i) {
            case 0: rowColor = RED; break;
            case 1: rowColor = ORANGE; break;
            case 2: rowColor = YELLOW; break;
            case 3: rowColor = GREEN; break;
            case 4: rowColor = SKYBLUE; break;
            default: rowColor = LIGHTGRAY; break;
        }

        for (int j = 0; j < bricksCols; j++) {
            bricks.emplace_back(
                (screenWidth / bricksCols) * j + 5, 
                50 * i + 50, 
                brickWidth, 
                brickHeight, 
                rowColor, 
                rowPoints
            );
        }
    }
}

void BreakoutGame::ResetBall() {
    ballPos = { player.x + player.width/2, player.y - ballRadius - 1 };
    ballSpeed = { 5.0f, -5.0f };
    gameStarted = false;
}

void BreakoutGame::ResetGame() {
    score = 0;
    lives = initialLives;
    currentState = GameState::MENU;
    InitBricks();
    ResetBall();
    std::cout << "Game Restarted! Returning to MENU..." << std::endl;
}

void BreakoutGame::Update() {
    // 状态机核心：switch 替代多 bool
    switch (currentState) {
        case GameState::MENU:
            // 菜单逻辑：按空格/点击进入游戏
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                currentState = GameState::PLAYING;
                gameStarted = true;
            }
            break;

        case GameState::PLAYING:
            // 游戏中逻辑
            if (IsKeyPressed(KEY_P)) {
                currentState = GameState::PAUSED;
                break;
            }

            // 生命≤0 → 游戏失败
            if (lives <= 0) {
                currentState = GameState::GAMEOVER;
                break;
            }

            // 砖块全清 → 胜利
            if (AllBricksCleared()) {
                currentState = GameState::VICTORY;
                break;
            }

            // 正常游戏更新逻辑
            if (!gameStarted) {
                if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    gameStarted = true;
                }
                ballPos.x = player.x + player.width/2;
                ballPos.y = player.y - ballRadius - 1;
            } else {
                ballPos.x += ballSpeed.x;
                ballPos.y += ballSpeed.y;
                
                // 边界反弹
                if (ballPos.x + ballRadius >= screenWidth || ballPos.x - ballRadius <= 0) {
                    ballSpeed.x *= -1;
                }
                if (ballPos.y - ballRadius <= 0) {
                    ballSpeed.y *= -1;
                }
                
                // 掉出底部 → 扣生命
                if (ballPos.y + ballRadius >= screenHeight) {
                    lives--;
                    std::cout << "Life lost! Remaining: " << lives << std::endl;
                    ResetBall();
                }
                
                // 挡板碰撞
                if (CheckCollisionCircleRec(ballPos, ballRadius, player)) {
                    ballSpeed.y *= -1;
                    ballPos.y = player.y - ballRadius - 1;
                }
                
                // 砖块碰撞
                for (auto& brick : bricks) {
                    int gained = brick.CheckHit(ballPos, ballRadius);
                    if (gained > 0) {
                        score += gained;
                        ballSpeed.y *= -1;
                        break;
                    }
                }
            }

            // 挡板跟随鼠标
            player.x = GetMouseX() - player.width/2;
            if (player.x <= 0) player.x = 0;
            if (player.x >= screenWidth - player.width) {
                player.x = screenWidth - player.width;
            }
            break;

        case GameState::PAUSED:
            // 暂停逻辑：按P继续
            if (IsKeyPressed(KEY_P)) {
                currentState = GameState::PLAYING;
            }
            break;

        case GameState::GAMEOVER:
        case GameState::VICTORY:
            // 结束界面：按R重置（回到菜单）
            if (IsKeyPressed(KEY_R)) {
                ResetGame();
            }
            break;

        case GameState::LEADERBOARD:
            // 暂未实现：按L切换
            if (IsKeyPressed(KEY_L)) {
                currentState = GameState::MENU;
            }
            break;
    }
}

void BreakoutGame::Draw() {
    BeginDrawing();
        ClearBackground(RAYWHITE);

        // 状态机绘制逻辑
        switch (currentState) {
            case GameState::MENU:
                DrawText("RAYLIB BREAKOUT", screenWidth/2 - 180, screenHeight/2 - 60, 40, DARKBLUE);
                DrawText("Press SPACE or CLICK to Start", screenWidth/2 - 180, screenHeight/2, 24, DARKGRAY);
                DrawText("Press P to Pause (in-game)", screenWidth/2 - 150, screenHeight/2 + 40, 20, GRAY);
                break;

            case GameState::PLAYING:
                DrawRectangleRec(player, DARKBLUE);
                DrawCircleV(ballPos, ballRadius, MAROON);
                
                for (const auto& brick : bricks) {
                    brick.Draw();
                }

                DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);
                DrawText(TextFormat("Lives: %d", lives), screenWidth - 100, 10, 20, DARKGRAY);
                break;

            case GameState::PAUSED:
                // 绘制游戏背景
                DrawRectangleRec(player, DARKBLUE);
                DrawCircleV(ballPos, ballRadius, MAROON);
                for (const auto& brick : bricks) {
                    brick.Draw();
                }
                DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);
                DrawText("PAUSED", screenWidth/2 - 80, screenHeight/2, 40, RED);
                DrawText("Press P to Resume", screenWidth/2 - 110, screenHeight/2 + 50, 20, GRAY);
                break;

            case GameState::GAMEOVER:
                // 绘制半透明遮罩
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
                DrawText("GAME OVER", screenWidth/2 - 140, screenHeight/2 - 40, 40, RED);
                DrawText(TextFormat("Final Score: %d", score), screenWidth/2 - 80, screenHeight/2 + 10, 20, WHITE);
                DrawText("Press R to Restart (Return to Menu)", screenWidth/2 - 150, screenHeight/2 + 50, 20, LIGHTGRAY);
                break;

            case GameState::VICTORY:
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(GREEN, 0.7f));
                DrawText("YOU WIN!", screenWidth/2 - 100, screenHeight/2 - 40, 40, GOLD);
                DrawText(TextFormat("Final Score: %d", score), screenWidth/2 - 80, screenHeight/2 + 10, 20, WHITE);
                DrawText("Press R to Restart (Return to Menu)", screenWidth/2 - 150, screenHeight/2 + 50, 20, LIGHTGRAY);
                break;

            case GameState::LEADERBOARD:
                DrawText("LEADERBOARD (Coming Soon)", screenWidth/2 - 150, screenHeight/2 - 40, 30, DARKGRAY);
                DrawText("Press L to Return to Menu", screenWidth/2 - 130, screenHeight/2 + 20, 20, GRAY);
                break;
        }
    EndDrawing();
}

void BreakoutGame::Run() {
    while (!WindowShouldClose()) {
        Update();
        Draw();
    }
    CloseWindow();
}