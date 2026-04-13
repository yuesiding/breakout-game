#include "breakoutgame.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>



// ==================== 美化配色定义（原生Color结构体，兼容所有raylib版本） ====================
const Color BG_COLOR = {18, 24, 36, 255};       // 深蓝灰背景
const Color PADDLE_COLOR = {66, 135, 245, 255}; // 浅蓝色挡板
const Color BALL_COLOR = {250, 250, 250, 255};  // 白色小球
const Color TEXT_COLOR = {240, 240, 240, 255};  // 浅灰文字
const Color GAME_OVER_BG = Fade((Color){255, 0, 0, 255}, 0.2f);  // 失败界面半透明背景
const Color WIN_BG = Fade((Color){0, 255, 0, 255}, 0.2f);        // 获胜界面半透明背景

// 砖块配色表（原生Color结构体）
const Color BRICK_COLORS[] = {
    {255, 105, 97, 255},   // 珊瑚红 (r,g,b,a)
    {100, 181, 246, 255},  // 浅蓝
    {129, 199, 132, 255},  // 薄荷绿
    {255, 202, 40, 255},   // 金黄
    {179, 157, 219, 255},  // 淡紫
    {250, 177, 160, 255}   // 蜜桃色
};
const int BRICK_COLOR_COUNT = sizeof(BRICK_COLORS)/sizeof(Color);

// ==================== Ball 实现 ====================
Ball::Ball(float x, float y, float sx, float sy, float r)
    : position{x, y}, speed{sx, sy}, radius{r}, slowTimer(0), slowFactor(1.0f) {
    srand(time(0)); // 初始化随机数
}

void Ball::Update() {
    if (slowTimer > 0) {
        slowTimer -= GetFrameTime();
        if (slowTimer <= 0) slowFactor = 1.0f;
    }
    position.x += speed.x * slowFactor * GetFrameTime();
    position.y += speed.y * slowFactor * GetFrameTime();
}

void Ball::Draw() const {
    DrawCircleV(position, radius, BALL_COLOR);
    // 小球描边
    DrawCircleLines(position.x, position.y, radius, Fade((Color){0, 0, 0, 255}, 0.5f));
}

void Ball::BounceX() { speed.x *= -1; }
void Ball::BounceY() { speed.y *= -1; }

void Ball::Slow(float factor, float time) {
    slowFactor = factor;
    slowTimer = time;
}

Ball* Ball::Clone() const {
    return new Ball(
        position.x, position.y,
        speed.x * 0.8f + (rand() % 20 - 10),
        speed.y * 0.8f + (rand() % 20 - 10),
        radius
    );
}

// ==================== Paddle 实现 ====================
Paddle::Paddle(float x, float y, float w, float h)
    : rect{x - w/2, y, w, h}, originalWidth(w), extendTimer(0) {}

void Paddle::Update() {
    // 鼠标控制挡板
    rect.x = GetMouseX() - rect.width / 2;
    if (rect.x < 0) rect.x = 0;
    if (rect.x + rect.width > 800) rect.x = 800 - rect.width;

    // 加长效果计时
    if (extendTimer > 0) {
        extendTimer -= GetFrameTime();
        if (extendTimer <= 0) rect.width = originalWidth;
    }
}

void Paddle::Draw() const {
    // 圆角挡板主体
    DrawRectangleRounded(rect, 0.2f, 8, PADDLE_COLOR);
    // 圆角描边（适配旧版raylib，4个参数）
    DrawRectangleRoundedLines(
        rect,           
        0.2f,           
        8,              
        Fade((Color){0, 0, 0, 255}, 0.5f)
    );
}

void Paddle::Extend(float extra, float time) {
    rect.width = originalWidth + extra;
    extendTimer = time;
}

// ==================== Brick 实现 ====================
Brick::Brick(float x, float y, float w, float h, Color col)
    : rect{x, y, w, h}, destroyed(false), color(col) {}

void Brick::Draw() const {
    if (!destroyed) {
        // 圆角砖块主体
        DrawRectangleRounded(rect, 0.1f, 4, color);
        // 圆角描边（适配旧版raylib，4个参数）
        DrawRectangleRoundedLines(
            rect,           
            0.1f,           
            4,              
            Fade((Color){0, 0, 0, 255}, 0.3f)
        );
        // 砖块高光
        Rectangle highlight = {rect.x, rect.y, rect.width, rect.height / 2.0f};
        DrawRectangleRounded(highlight, 0.1f, 4, Fade((Color){255, 255, 255, 255}, 0.2f));
    }
}

// ==================== BreakoutGame 实现 ====================
// 1. 构造函数初始化
BreakoutGame::BreakoutGame() 
    : paddle(nullptr), score(0), life(5), gameState(GameState::START), isBallPaused(false) {
    InitWindow(800, 600, "Breakout Game");
    SetTargetFPS(60);
    InitGame();
}


BreakoutGame::~BreakoutGame() {
    Cleanup();
    CloseWindow();
}

// 2. InitGame初始化
void BreakoutGame::InitGame() {
    paddle = new Paddle(400, 550, 100, 20);
    // 球初始速度0（静止），位置在挡板上方
    balls.push_back(new Ball(400.0f, 530.0f, 0.0f, 0.0f, 8.0f)); 
    isBallPaused = true; // 初始就暂停（起始界面球停挡板上）
    // 砖块初始化（原有代码不变）
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 10; x++) {
            int colorIndex = (x + y) % BRICK_COLOR_COUNT;
            Color brickColor = BRICK_COLORS[colorIndex];
            float brickX = 65.0f + (float)x * 70.0f;
            float brickY = 60.0f + (float)y * 35.0f;
            float brickW = 60.0f;
            float brickH = 25.0f;
            Brick* newBrick = new Brick(brickX, brickY, brickW, brickH, brickColor);
            bricks.push_back(newBrick);
        }
    }
}

void BreakoutGame::SpawnParticles(Vector2 pos, Color color) {
    // 生成彩色粒子
    for (int i = 0; i < 12; i++) {
        particles.push_back({
            pos,
            {(float)(rand()%120-60)/10.0f, (float)(rand()%120-60)/10.0f},
            color,
            0.6f
        });
    }
}

void BreakoutGame::SpawnPowerUp(Vector2 pos) {
    // 随机生成道具类型
    int type = rand() % 3;
    powerUps.emplace_back(pos.x, pos.y, (PowerUpType)type);
}

// 判断是否获胜（所有砖块被打碎）
bool BreakoutGame::CheckWin() {
    for (auto& brick : bricks) {
        if (!brick->IsDestroyed()) return false;
    }
    return true;
}

void BreakoutGame::HandleCollisions() {
    // 1. 球-墙壁碰撞（原有代码不变）
    for (auto& ball : balls) {
        Vector2 p = ball->GetPosition();
        if (p.x < 8 || p.x > 792) ball->BounceX();
        if (p.y < 8) ball->BounceY();
    }

    // 2. 球-挡板碰撞（原有代码不变）
    for (auto& ball : balls) {
        if (CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), paddle->GetRect())) {
            ball->BounceY();
            float hitPos = (ball->GetPosition().x - paddle->GetRect().x) / paddle->GetRect().width;
            Vector2 currentSpeed = ball->GetSpeed();
            ball->SetSpeed({(hitPos - 0.5f) * 500.0f, currentSpeed.y});
        }
    }

    // 3. 球-砖块碰撞（原有代码不变）
    std::vector<bool> hit(bricks.size(), false);
    for (auto& ball : balls) {
        for (size_t i = 0; i < bricks.size(); i++) {
            Brick* brick = bricks[i];
            if (!brick->IsDestroyed() && !hit[i] &&
                CheckCollisionCircleRec(ball->GetPosition(), ball->GetRadius(), brick->GetRect())) {
                ball->BounceY();
                brick->Destroy();
                score += 10;
                hit[i] = true;
                SpawnParticles(brick->GetPosition(), brick->GetColor());
                if (rand() % 100 < 30) {
                    SpawnPowerUp(brick->GetPosition());
                }
            }
        }
    }

    // 4. 挡板-道具碰撞（原有代码不变）
    for (auto& pu : powerUps) {
        if (pu.CheckCollision(paddle->GetRect())) {
            pu.Activate();
            switch (pu.GetType()) {
                case PowerUpType::PADDLE_EXTEND:
                    paddle->Extend(60.0f, 5.0f); 
                    break;
                case PowerUpType::MULTI_BALL: {  
                    std::vector<Ball*> newBalls;
                    for (auto& ball : balls) {
                        newBalls.push_back(ball->Clone());
                    }
                    for (auto& ball : newBalls) {
                        balls.push_back(ball);
                    }
                    break;
                }
                case PowerUpType::SLOW_BALL:
                    for (auto& ball : balls) {
                        ball->Slow(0.5f, 5.0f);
                    }
                    break;
            }
        }
    }

    // 5. 球出界处理（终极修复版！）
    for (auto it = balls.begin(); it != balls.end();) {
        if ((*it)->GetPosition().y > 600) { // 球掉出屏幕下方
            if (balls.size() == 1) {
                // 只剩1个球：扣血+停挡板上+标记暂停
                life--;
                Rectangle paddleRect = paddle->GetRect();
                float paddleCenterX = paddleRect.x + paddleRect.width / 2.0f;
                // 球的位置：挡板中心x + 挡板上方20像素（550-20=530）
                (*it)->SetPosition({paddleCenterX, paddleRect.y - 20.0f});
                (*it)->SetSpeed({0.0f, 0.0f}); // 速度0，完全静止
                isBallPaused = true; // 标记为暂停
                it++;
            } else {
                // 多个球：删除出界的球
                delete *it;
                it = balls.erase(it);
            }
        } else {
            it++;
        }
    }

    // 清理道具/粒子（原有代码不变）
    powerUps.erase(
        std::remove_if(powerUps.begin(), powerUps.end(),
            [](const PowerUp& pu) { return !pu.IsActive(); }),
        powerUps.end()
    );
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.IsDead(); }),
        particles.end()
    );

    // 更新游戏状态（原有代码不变）
    if (life <= 0) {
        gameState = GameState::GAME_OVER;
    } else if (CheckWin()) {
        gameState = GameState::WIN;
    }
}

void BreakoutGame::UpdateGame() {
    // 1. 起始界面逻辑（球初始停挡板上，按Space仅切换状态，不发射）
    if (gameState == GameState::START) {
        // 起始界面：球始终停挡板上，随挡板移动
        paddle->Update();
        if (!balls.empty()) {
            Rectangle paddleRect = paddle->GetRect();
            float paddleCenterX = paddleRect.x + paddleRect.width / 2.0f;
            Vector2 ballPos = balls[0]->GetPosition();
            balls[0]->SetPosition({paddleCenterX, paddleRect.y - 20.0f});
        }

        // 按Space仅进入游戏状态，不发射球（保持暂停）
        if (IsKeyPressed(KEY_SPACE)) {
            gameState = GameState::PLAYING;
            isBallPaused = true; // 进入游戏后仍暂停，等待鼠标发射
        }
        return;
    }

    // 2. 失败/获胜重启逻辑（原有代码不变）
    if (gameState == GameState::GAME_OVER || gameState == GameState::WIN) {
        if (IsKeyPressed(KEY_R)) {
            Cleanup();
            score = 0;
            life = 5;
            gameState = GameState::START;
            InitGame();
            isBallPaused = true; // 重启后球暂停
        }
        return;
    }

    // 3. 游戏中逻辑（核心！）
    paddle->Update(); // 始终更新挡板

    // 4. 球暂停：停挡板上+随挡板移动+鼠标左键发射
    if (isBallPaused && !balls.empty()) {
        Rectangle paddleRect = paddle->GetRect();
        float paddleCenterX = paddleRect.x + paddleRect.width / 2.0f;
        // 强制球的位置：挡板中心x + 挡板上方20像素（固定y）
        balls[0]->SetPosition({paddleCenterX, paddleRect.y - 20.0f});
        balls[0]->SetSpeed({0.0f, 0.0f}); // 确保速度为0，不漂移

        // 鼠标左键：发射球（向上，速度250）
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            balls[0]->SetSpeed({0.0f, -250.0f}); // 纯向上发射，不左右偏
            isBallPaused = false; // 取消暂停
        }
    } 
    // 5. 球未暂停：正常更新
    else {
        for (auto& ball : balls) {
            ball->Update();
        }
    }

    // 6. 更新道具/粒子（原有代码不变）
    for (auto& pu : powerUps) pu.Update();
    for (auto& p : particles) p.Update();

    // 7. 碰撞检测
    HandleCollisions();
}

void BreakoutGame::DrawGame() {
    BeginDrawing();
    ClearBackground(BG_COLOR);

    // 绘制网格背景，增加层次感
    for (int x = 0; x < screenWidth; x += 20) {
        DrawLine((float)x, 0.0f, (float)x, (float)screenHeight, Fade((Color){255,255,255,255}, 0.05f));
    }
    for (int y = 0; y < screenHeight; y += 20) {
        DrawLine(0.0f, (float)y, (float)screenWidth, (float)y, Fade((Color){255,255,255,255}, 0.05f));
    }

    // 绘制游戏元素（起始界面也显示挡板/砖块，仅球静止）
    paddle->Draw();
    for (auto& ball : balls) ball->Draw();
    for (auto& brick : bricks) brick->Draw();
    for (auto& p : particles) p.Draw();
    for (auto& pu : powerUps) pu.Draw();

    // 绘制UI：分数和生命值（所有状态都显示）
    DrawText(TextFormat("SCORE: %04d", score), 20, 20, 24, TEXT_COLOR);
    DrawText(TextFormat("LIFE: %d", life), 700, 20, 24, TEXT_COLOR);

    // 1. 起始界面提示（核心新增）
    if (gameState == GameState::START) {
        // 半透明黑色遮罩，突出提示文字
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade((Color){0,0,0,255}, 0.5f));
        // 游戏标题
        DrawText("BREAKOUT GAME", 220, 200, 48, (Color){255,255,255,255});
        // 开始提示
        DrawText("PRESS SPACE TO START", 250, 300, 28, (Color){255,255,255,255});
        // 操作提示（可选，增加友好性）
        DrawText("MOVE MOUSE TO CONTROL PADDLE", 220, 350, 24, Fade((Color){255,255,255,255}, 0.8f));
    }
    // 2. 游戏失败提示
    else if (gameState == GameState::GAME_OVER) {
        DrawRectangle(0, 0, screenWidth, screenHeight, GAME_OVER_BG);
        DrawText("GAME OVER", 280, 250, 52, (Color){255,0,0,255});
        DrawText(TextFormat("FINAL SCORE: %04d", score), 280, 320, 32, TEXT_COLOR);
        DrawText("PRESS R TO RESTART", 290, 380, 28, TEXT_COLOR);
    }
    // 3. 游戏获胜提示
    else if (gameState == GameState::WIN) {
        DrawRectangle(0, 0, screenWidth, screenHeight, WIN_BG);
        DrawText("YOU WIN!", 300, 250, 52, (Color){0,255,0,255});
        DrawText(TextFormat("FINAL SCORE: %04d", score), 280, 320, 32, TEXT_COLOR);
        DrawText("PRESS R TO PLAY AGAIN", 270, 380, 28, TEXT_COLOR);
    }

    EndDrawing();
}

void BreakoutGame::Cleanup() {
    // 释放内存
    delete paddle;
    for (auto& ball : balls) delete ball;
    for (auto& brick : bricks) delete brick;
    balls.clear();
    bricks.clear();
    particles.clear();
    powerUps.clear();
}

void BreakoutGame::Run() {
    // 游戏主循环
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }
}