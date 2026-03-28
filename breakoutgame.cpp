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
    InitWindow(screenWidth, screenHeight, "Raylib C++ 打砖块（生命值版）");
    
    player = { (float)screenWidth/2 - 50, (float)screenHeight - 50, 100, 15 };
    
    ballRadius = 8.0f;
    
    score = 0;
    lives = 5;
    gameOver = false;
    gameStarted = false;
    
    InitBricks();
    ResetBall();

    SetTargetFPS(60);
}

void BreakoutGame::InitBricks() {
    bricks.clear();
    for (int i = 0; i < 5; i++) {
        Color rowColor;
        int rowPoints = (5 - i) * 10;
        
        switch (i) {
            case 0: rowColor = RED; break;
            case 1: rowColor = ORANGE; break;
            case 2: rowColor = YELLOW; break;
            case 3: rowColor = GREEN; break;
            case 4: rowColor = SKYBLUE; break;
            default: rowColor = LIGHTGRAY; break;
        }

        for (int j = 0; j < 10; j++) {
            bricks.emplace_back(80*j + 5, 50*i + 50, 70, 25, rowColor, rowPoints);
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
    lives = 5;
    gameOver = false;
    InitBricks();
    ResetBall();
    std::cout << "Game Restarted!" << std::endl;
}

void BreakoutGame::Update() {
    if (gameOver) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ResetGame();
        }
        return;
    }

    if (!gameStarted) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            gameStarted = true;
        }
        ballPos.x = player.x + player.width/2;
        ballPos.y = player.y - ballRadius - 1;
    } else {
        ballPos.x += ballSpeed.x;
        ballPos.y += ballSpeed.y;
        
        if (ballPos.x + ballRadius >= screenWidth || ballPos.x - ballRadius <= 0) {
            ballSpeed.x *= -1;
        }
        if (ballPos.y - ballRadius <= 0) {
            ballSpeed.y *= -1;
        }
        
        if (ballPos.y + ballRadius >= screenHeight) {
            lives--;
            std::cout << "Life lost! Remaining: " << lives << std::endl;
            
            if (lives <= 0) {
                gameOver = true;
                std::cout << "GAME OVER! Final Score: " << score << std::endl;
            } else {
                ResetBall();
            }
        }
        
        if (CheckCollisionCircleRec(ballPos, ballRadius, player)) {
            ballSpeed.y *= -1;
            ballPos.y = player.y - ballRadius - 1;
        }
        
        for (auto& brick : bricks) {
            int gained = brick.CheckHit(ballPos, ballRadius);
            if (gained > 0) {
                score += gained;
                ballSpeed.y *= -1;
                break;
            }
        }
    }

    player.x = GetMouseX() - player.width/2;
    if (player.x <= 0) player.x = 0;
    if (player.x >= screenWidth - player.width) {
        player.x = screenWidth - player.width;
    }
}

void BreakoutGame::Draw() {
    BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRec(player, DARKBLUE);
        DrawCircleV(ballPos, ballRadius, MAROON);
        
        for (const auto& brick : bricks) {
            brick.Draw();
        }

        DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Lives: %d", lives), screenWidth - 100, 10, 20, DARKGRAY);

        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
            DrawText("GAME OVER", screenWidth/2 - 100, screenHeight/2 - 40, 40, RED);
            DrawText(TextFormat("Final Score: %d", score), screenWidth/2 - 80, screenHeight/2 + 10, 20, WHITE);
            DrawText("Press SPACE to Restart", screenWidth/2 - 110, screenHeight/2 + 50, 20, LIGHTGRAY);
        } else if (!gameStarted) {
            DrawText("Press SPACE or CLICK to Start", screenWidth/2 - 180, screenHeight/2, 24, DARKGRAY);
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